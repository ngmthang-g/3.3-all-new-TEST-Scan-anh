import { createClient } from "npm:@supabase/supabase-js@2";

const LICENSE_DAYS = new Set([3, 15, 30, 60, 90, 365]);
const KEY_ALPHABET = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
const MAX_KEYS = 500;
const MAX_USAGE_DAYS = 60;

const corsHeaders = {
  "Access-Control-Allow-Origin": "*",
  "Access-Control-Allow-Headers": "authorization, apikey, content-type",
  "Access-Control-Allow-Methods": "POST, OPTIONS",
  "Cache-Control": "no-store",
  "X-Content-Type-Options": "nosniff",
};

function envSecretKey(): string {
  const legacy = Deno.env.get("SUPABASE_SERVICE_ROLE_KEY") || "";
  if (legacy) return legacy;
  try {
    const parsed = JSON.parse(Deno.env.get("SUPABASE_SECRET_KEYS") || "{}");
    return String(parsed.default || Object.values(parsed)[0] || "");
  } catch {
    return "";
  }
}

const supabaseUrl = Deno.env.get("SUPABASE_URL") || "";
const serviceKey = envSecretKey();
if (!supabaseUrl || !serviceKey) throw new Error("Supabase backend secrets are unavailable.");

const db = createClient(supabaseUrl, serviceKey, {
  auth: { persistSession: false, autoRefreshToken: false },
});

function json(data: unknown, status = 200): Response {
  return new Response(JSON.stringify(data), {
    status,
    headers: { ...corsHeaders, "Content-Type": "application/json; charset=utf-8" },
  });
}

function cleanString(value: unknown, maxLen: number): string {
  return typeof value === "string" ? value.trim().slice(0, maxLen) : "";
}

function normalizeKey(value: unknown): string {
  return cleanString(value, 128).toUpperCase();
}

async function sha256(value: string): Promise<string> {
  const bytes = new TextEncoder().encode(value);
  const digest = await crypto.subtle.digest("SHA-256", bytes);
  return [...new Uint8Array(digest)].map((b) => b.toString(16).padStart(2, "0")).join("");
}

function generateKey(): string {
  const bytes = new Uint8Array(25);
  crypto.getRandomValues(bytes);
  let body = "";
  for (const byte of bytes) body += KEY_ALPHABET[byte % KEY_ALPHABET.length];
  return `TLPRO-${body.slice(0, 5)}-${body.slice(5, 10)}-${body.slice(10, 15)}-${body.slice(15, 20)}-${body.slice(20, 25)}`;
}

const PROFILE_HASH_FIELDS = [
  "machineGuidHash", "systemVolumeHash", "cpuHash", "systemProductHash",
  "baseBoardHash", "biosHash", "firmwareHash",
] as const;

function sanitizeDeviceProfile(value: unknown): Record<string, unknown> {
  if (!value || typeof value !== "object" || Array.isArray(value)) return {};
  const input = value as Record<string, unknown>;
  const out: Record<string, unknown> = {};
  if (Number(input.fingerprintVersion) === 2) out.fingerprintVersion = 2;
  for (const field of PROFILE_HASH_FIELDS) {
    const hash = cleanString(input[field], 64).toLowerCase();
    if (/^[a-f0-9]{64}$/.test(hash)) out[field] = hash;
  }
  return out;
}

function clientInput(body: Record<string, unknown>) {
  const key = normalizeKey(body.key);
  const deviceHash = cleanString(body.deviceHash, 128).toLowerCase();
  const legacyDeviceHash = cleanString(body.legacyDeviceHash, 128).toLowerCase();
  const deviceProfile = sanitizeDeviceProfile(body.deviceProfile);
  const machineName = cleanString(body.machineName, 120);
  const sessionId = cleanString(body.sessionId, 80).toLowerCase();
  const appVersion = cleanString(body.appVersion, 32);
  if (!/^TLPRO-[A-Z2-9-]{20,64}$/.test(key)) throw new Error("invalid_key_format");
  if (!/^[a-f0-9]{64}$/.test(deviceHash)) throw new Error("invalid_device_hash");
  if (legacyDeviceHash && !/^[a-f0-9]{64}$/.test(legacyDeviceHash)) throw new Error("invalid_legacy_device_hash");
  if (!/^[a-f0-9]{32,64}$/.test(sessionId)) throw new Error("invalid_session");
  return { key, deviceHash, legacyDeviceHash, deviceProfile, machineName, sessionId, appVersion };
}

async function requireAdmin(req: Request) {
  const header = req.headers.get("authorization") || "";
  const match = /^Bearer\s+(.+)$/i.exec(header);
  if (!match) return { error: json({ ok: false, message: "Chưa đăng nhập admin." }, 401) };

  const { data: authData, error: authError } = await db.auth.getUser(match[1]);
  const user = authData?.user;
  if (authError || !user) return { error: json({ ok: false, message: "Phiên đăng nhập admin không hợp lệ." }, 401) };

  const { data: adminRow, error: adminError } = await db
    .from("license_admins")
    .select("enabled")
    .eq("user_id", user.id)
    .maybeSingle();
  if (adminError || !adminRow || adminRow.enabled !== true) {
    return { error: json({ ok: false, message: "Tài khoản không có quyền admin." }, 403) };
  }
  return { user };
}

function licenseStatus(result: any): number {
  return result && result.ok === true ? 200 : 403;
}

function mapLicense(row: any) {
  return {
    keyHash: row.key_hash,
    key: row.key_text,
    durationDays: Number(row.duration_days || 0),
    status: row.status || "blocked",
    note: row.note || "",
    machineHash: row.machine_hash || "",
    legacyMachineHash: row.legacy_machine_hash || "",
    machineProfile: row.machine_profile || {},
    machineName: row.machine_name || "",
    mismatchMachineHash: row.mismatch_machine_hash || "",
    mismatchMachineName: row.mismatch_machine_name || "",
    mismatchMachineProfile: row.mismatch_machine_profile || {},
    blockedAt: row.blocked_at || null,
    blockedReason: row.blocked_reason || "",
    createdAt: row.created_at || null,
    activatedAt: row.activated_at || null,
    expiresAt: row.expires_at || null,
    lastSeenAt: row.last_seen_at || null,
    launchCount: Number(row.launch_count || 0),
    lastAppVersion: row.last_app_version || "",
    todayDate: row.today_date || "",
    todayActiveSeconds: Number(row.today_active_seconds || 0),
  };
}

Deno.serve(async (req) => {
  if (req.method === "OPTIONS") return new Response(null, { status: 204, headers: corsHeaders });
  if (req.method !== "POST") return json({ ok: false, message: "Method not allowed." }, 405);

  const pathname = new URL(req.url).pathname;
  let body: Record<string, unknown> = {};
  try {
    body = await req.json();
  } catch {
    return json({ ok: false, message: "JSON không hợp lệ." }, 400);
  }

  try {
    if (pathname.endsWith("/validate")) {
      const input = clientInput(body);
      const keyHash = await sha256(input.key);
      const { data, error } = await db.rpc("license_validate_v2", {
        p_key_hash: keyHash,
        p_device_hash: input.deviceHash,
        p_legacy_device_hash: input.legacyDeviceHash,
        p_machine_profile: input.deviceProfile,
        p_machine_name: input.machineName,
        p_session_id: input.sessionId,
        p_app_version: input.appVersion,
      });
      if (error) {
        console.error("license_validate_v2", error);
        return json({ ok: false, message: "Server license đang lỗi." }, 500);
      }
      return json(data, licenseStatus(data));
    }

    if (pathname.endsWith("/heartbeat")) {
      const input = clientInput(body);
      const keyHash = await sha256(input.key);
      const { data, error } = await db.rpc("license_heartbeat_v2", {
        p_key_hash: keyHash,
        p_device_hash: input.deviceHash,
        p_legacy_device_hash: input.legacyDeviceHash,
        p_machine_profile: input.deviceProfile,
        p_machine_name: input.machineName,
        p_session_id: input.sessionId,
        p_app_version: input.appVersion,
      });
      if (error) {
        console.error("license_heartbeat_v2", error);
        return json({ ok: false, message: "Server license đang lỗi." }, 500);
      }
      return json(data, licenseStatus(data));
    }

    if (pathname.endsWith("/admin/createKey")) {
      const admin = await requireAdmin(req);
      if (admin.error) return admin.error;
      const durationDays = Number(body.durationDays);
      const note = cleanString(body.note, 240);
      if (!LICENSE_DAYS.has(durationDays)) return json({ ok: false, message: "Loại key không hợp lệ." }, 400);

      for (let attempt = 0; attempt < 8; attempt += 1) {
        const key = generateKey();
        const keyHash = await sha256(key);
        const { error } = await db.from("licenses").insert({
          key_hash: keyHash,
          key_text: key,
          duration_days: durationDays,
          status: "allowed",
          note,
          created_by_user: admin.user.id,
          created_by_email: admin.user.email || "",
        });
        if (!error) return json({ ok: true, message: "Đã tạo key.", key, keyHash, durationDays });
        if (error.code !== "23505") {
          console.error("createKey", error);
          return json({ ok: false, message: "Không tạo được key." }, 500);
        }
      }
      return json({ ok: false, message: "Không tạo được key." }, 500);
    }

    if (pathname.endsWith("/admin/listKeys")) {
      const admin = await requireAdmin(req);
      if (admin.error) return admin.error;
      const { data, error } = await db
        .from("licenses")
        .select("key_hash,key_text,duration_days,status,note,machine_hash,legacy_machine_hash,machine_profile,machine_name,mismatch_machine_hash,mismatch_machine_name,mismatch_machine_profile,blocked_at,blocked_reason,created_at,activated_at,expires_at,last_seen_at,launch_count,last_app_version,today_date,today_active_seconds")
        .order("created_at", { ascending: false })
        .limit(MAX_KEYS);
      if (error) {
        console.error("listKeys", error);
        return json({ ok: false, message: "Không tải được danh sách key." }, 500);
      }
      return json({ ok: true, keys: (data || []).map(mapLicense) });
    }

    if (pathname.endsWith("/admin/updateKey")) {
      const admin = await requireAdmin(req);
      if (admin.error) return admin.error;
      const keyHash = cleanString(body.keyHash, 80).toLowerCase();
      if (!/^[a-f0-9]{64}$/.test(keyHash)) return json({ ok: false, message: "KeyHash không hợp lệ." }, 400);
      const patch: Record<string, unknown> = {};
      if (Object.prototype.hasOwnProperty.call(body, "status")) {
        const status = cleanString(body.status, 16);
        if (status !== "allowed" && status !== "blocked") return json({ ok: false, message: "Trạng thái không hợp lệ." }, 400);
        patch.status = status;
        if (status === "blocked") {
          patch.blocked_at = new Date().toISOString();
          patch.blocked_reason = "admin";
        } else {
          patch.blocked_at = null;
          patch.blocked_reason = "";
        }
      }
      if (Object.prototype.hasOwnProperty.call(body, "note")) patch.note = cleanString(body.note, 240);
      if (!Object.keys(patch).length) return json({ ok: false, message: "Không có dữ liệu cần sửa." }, 400);
      const { error } = await db.from("licenses").update(patch).eq("key_hash", keyHash);
      if (error) {
        console.error("updateKey", error);
        return json({ ok: false, message: "Không cập nhật được key." }, 500);
      }
      return json({ ok: true, message: "Đã cập nhật key." });
    }

    if (pathname.endsWith("/admin/deleteKey")) {
      const admin = await requireAdmin(req);
      if (admin.error) return admin.error;
      const keyHash = cleanString(body.keyHash, 80).toLowerCase();
      if (!/^[a-f0-9]{64}$/.test(keyHash)) return json({ ok: false, message: "KeyHash không hợp lệ." }, 400);

      const { data: row, error: readError } = await db
        .from("licenses")
        .select("status,key_text")
        .eq("key_hash", keyHash)
        .maybeSingle();
      if (readError) {
        console.error("deleteKey/read", readError);
        return json({ ok: false, message: "Không kiểm tra được key." }, 500);
      }
      if (!row) return json({ ok: false, message: "Key không tồn tại." }, 404);
      if (row.status !== "blocked") {
        return json({ ok: false, message: "Chỉ được xóa key đã bị chặn." }, 409);
      }

      const { error: deleteError } = await db.from("licenses").delete().eq("key_hash", keyHash);
      if (deleteError) {
        console.error("deleteKey", deleteError);
        return json({ ok: false, message: "Không xóa được key." }, 500);
      }
      return json({ ok: true, message: "Đã xóa vĩnh viễn key bị chặn." });
    }

    if (pathname.endsWith("/admin/usage")) {
      const admin = await requireAdmin(req);
      if (admin.error) return admin.error;
      const keyHash = cleanString(body.keyHash, 80).toLowerCase();
      if (!/^[a-f0-9]{64}$/.test(keyHash)) return json({ ok: false, message: "KeyHash không hợp lệ." }, 400);
      const { data, error } = await db
        .from("license_daily")
        .select("day,first_seen_at,last_seen_at,launches,active_seconds,machine_name,app_version")
        .eq("key_hash", keyHash)
        .order("day", { ascending: false })
        .limit(MAX_USAGE_DAYS);
      if (error) {
        console.error("usage", error);
        return json({ ok: false, message: "Không tải được báo cáo." }, 500);
      }
      return json({
        ok: true,
        usage: (data || []).map((row: any) => ({
          date: row.day,
          firstSeenAt: row.first_seen_at,
          lastSeenAt: row.last_seen_at,
          launches: Number(row.launches || 0),
          activeSeconds: Number(row.active_seconds || 0),
          machineName: row.machine_name || "",
          appVersion: row.app_version || "",
        })),
      });
    }

    return json({ ok: false, message: "Route không tồn tại." }, 404);
  } catch (error) {
    console.error("license-api", error);
    return json({ ok: false, message: "Dữ liệu license không hợp lệ." }, 400);
  }
});
