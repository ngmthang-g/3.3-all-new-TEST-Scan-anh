import { createClient } from "https://cdn.jsdelivr.net/npm/@supabase/supabase-js@2/+esm";
import { supabaseConfig } from "./supabase-config.js";

const $ = (id) => document.getElementById(id);
const loginCard = $("loginCard");
const adminApp = $("adminApp");
const loginStatus = $("loginStatus");
const adminStatus = $("adminStatus");
const keyRows = $("keyRows");
const usageCard = $("usageCard");
const usageTitle = $("usageTitle");
const usageBody = $("usageBody");

function esc(value) {
  return String(value ?? "").replace(/[&<>"']/g, (ch) => ({
    "&": "&amp;", "<": "&lt;", ">": "&gt;", "\"": "&quot;", "'": "&#39;"
  })[ch]);
}

function formatDate(value) {
  if (!value) return "-";
  const date = new Date(value);
  if (Number.isNaN(date.getTime())) return "-";
  return date.toLocaleString("vi-VN", { hour12: false });
}

function formatSeconds(total) {
  let seconds = Math.max(0, Number(total || 0));
  const hours = Math.floor(seconds / 3600);
  seconds -= hours * 3600;
  const minutes = Math.floor(seconds / 60);
  return hours > 0 ? `${hours}g ${minutes}p` : `${minutes}p`;
}

function profileTitle(profile) {
  if (!profile || typeof profile !== "object") return "";
  const fields = [
    ["MachineGuid", profile.machineGuidHash],
    ["Volume", profile.systemVolumeHash],
    ["CPU", profile.cpuHash],
    ["System", profile.systemProductHash],
    ["Board", profile.baseBoardHash],
    ["BIOS", profile.biosHash],
    ["Firmware", profile.firmwareHash],
  ];
  return fields.filter(([, value]) => value).map(([name, value]) => `${name}: ${String(value).slice(0, 16)}…`).join("\n");
}

function blockReasonText(item) {
  if (item.status !== "blocked") return "";
  if (item.blockedReason === "machine_mismatch") return "Tự chặn: phát hiện PC khác";
  if (item.blockedReason === "admin") return "Admin chặn";
  return "Đã chặn";
}

function setAdminStatus(message) { adminStatus.textContent = message || ""; }

let supabase = null;
try {
  if (String(supabaseConfig.url || "").startsWith("REPLACE_") ||
      String(supabaseConfig.publishableKey || "").startsWith("REPLACE_")) {
    throw new Error("Chưa điền supabase/web/supabase-config.js");
  }
  supabase = createClient(supabaseConfig.url, supabaseConfig.publishableKey, {
    auth: { persistSession: true, autoRefreshToken: true, detectSessionInUrl: false }
  });
} catch (error) {
  loginStatus.textContent = error.message || "Supabase config không hợp lệ.";
}

async function currentSession() {
  if (!supabase) return null;
  const { data } = await supabase.auth.getSession();
  return data.session || null;
}

async function api(path, body = {}) {
  const session = await currentSession();
  if (!session) throw new Error("Chưa đăng nhập admin.");
  const base = String(supabaseConfig.functionBaseUrl || "").replace(/\/+$/, "");
  if (!base || base.startsWith("REPLACE_")) throw new Error("Chưa cấu hình URL Edge Function.");
  const response = await fetch(`${base}${path}`, {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
      "Authorization": `Bearer ${session.access_token}`,
      "apikey": supabaseConfig.publishableKey
    },
    body: JSON.stringify(body)
  });
  const data = await response.json().catch(() => ({}));
  if (!response.ok || data.ok === false) throw new Error(data.message || `HTTP ${response.status}`);
  return data;
}

function renderKeys(keys) {
  keyRows.innerHTML = keys.map((item) => {
    const allowed = item.status === "allowed";
    const machine = item.machineName || (item.machineHash ? item.machineHash.slice(0, 12) : "Chưa kích hoạt");
    const fpTitle = profileTitle(item.machineProfile);
    const blockedReason = blockReasonText(item);
    const mismatch = item.mismatchMachineName ? `PC lạ: ${item.mismatchMachineName}` : "";
    return `<tr data-hash="${esc(item.keyHash)}">
      <td><span class="key">${esc(item.key)}</span><br><button class="small secondary" data-action="copy">Copy</button></td>
      <td>${esc(item.durationDays)} ngày</td>
      <td><span class="pill ${allowed ? "ok" : "blocked"}">${allowed ? "CHO PHÉP" : "ĐÃ CHẶN"}</span>${blockedReason ? `<br><span class="muted">${esc(blockedReason)}</span>` : ""}${mismatch ? `<br><span class="muted">${esc(mismatch)}</span>` : ""}</td>
      <td title="${esc(`${item.machineHash || ""}\n${fpTitle}`)}">${esc(machine)}${item.machineProfile?.fingerprintVersion ? `<br><span class="muted">FP v${esc(item.machineProfile.fingerprintVersion)}</span>` : ""}</td>
      <td>${esc(formatDate(item.activatedAt))}</td>
      <td>${esc(formatDate(item.expiresAt))}</td>
      <td>${esc(formatSeconds(item.todayActiveSeconds))}<br><span class="muted">${esc(item.todayDate || "-")}</span></td>
      <td>${esc(formatDate(item.lastSeenAt))}<br><span class="muted">Mở: ${esc(item.launchCount)} • v${esc(item.lastAppVersion || "-")}</span></td>
      <td><input class="noteInput" maxlength="240" value="${esc(item.note)}"></td>
      <td><div class="actions">
        <button class="small secondary" data-action="save">Lưu</button>
        <button class="small ${allowed ? "danger" : "allow"}" data-action="toggle" data-status="${allowed ? "blocked" : "allowed"}">${allowed ? "Chặn" : "Cho phép"}</button>
        <button class="small secondary" data-action="usage">Báo cáo</button>
        ${allowed ? "" : '<button class="small danger" data-action="delete">Xóa</button>'}
      </div></td>
    </tr>`;
  }).join("");
}

async function loadKeys() {
  setAdminStatus("Đang tải...");
  try {
    const data = await api("/admin/listKeys");
    renderKeys(data.keys || []);
    setAdminStatus(`Có ${(data.keys || []).length} key.`);
  } catch (error) {
    setAdminStatus(error.message);
  }
}

async function renderAuthState() {
  const session = await currentSession();
  const loggedIn = Boolean(session);
  loginCard.classList.toggle("hidden", loggedIn);
  adminApp.classList.toggle("hidden", !loggedIn);
  usageCard.classList.add("hidden");
  if (loggedIn) await loadKeys();
  else keyRows.innerHTML = "";
}

$("loginBtn").addEventListener("click", async () => {
  if (!supabase) return;
  loginStatus.textContent = "Đang đăng nhập...";
  const { error } = await supabase.auth.signInWithPassword({
    email: $("email").value.trim(),
    password: $("password").value
  });
  $("password").value = "";
  loginStatus.textContent = error ? (error.message || "Đăng nhập thất bại.") : "";
  if (!error) await renderAuthState();
});

$("password").addEventListener("keydown", (event) => {
  if (event.key === "Enter") $("loginBtn").click();
});

$("logoutBtn").addEventListener("click", async () => {
  if (supabase) await supabase.auth.signOut();
  await renderAuthState();
});

$("createBtn").addEventListener("click", async () => {
  const button = $("createBtn");
  button.disabled = true;
  setAdminStatus("Đang tạo key...");
  try {
    const data = await api("/admin/createKey", {
      durationDays: Number($("duration").value),
      note: $("newNote").value.trim()
    });
    $("newNote").value = "";
    setAdminStatus(`Đã tạo: ${data.key}`);
    try { await navigator.clipboard.writeText(data.key); } catch (_) {}
    await loadKeys();
  } catch (error) {
    setAdminStatus(error.message);
  } finally {
    button.disabled = false;
  }
});

keyRows.addEventListener("click", async (event) => {
  const button = event.target.closest("button[data-action]");
  if (!button) return;
  const row = button.closest("tr[data-hash]");
  if (!row) return;
  const keyHash = row.dataset.hash;
  const action = button.dataset.action;

  if (action === "copy") {
    try {
      await navigator.clipboard.writeText(row.querySelector(".key").textContent);
      setAdminStatus("Đã copy key.");
    } catch (_) { setAdminStatus("Trình duyệt không cho copy tự động."); }
    return;
  }

  if (action === "save") {
    try {
      await api("/admin/updateKey", { keyHash, note: row.querySelector(".noteInput").value });
      setAdminStatus("Đã lưu ghi chú.");
    } catch (error) { setAdminStatus(error.message); }
    return;
  }

  if (action === "toggle") {
    try {
      await api("/admin/updateKey", { keyHash, status: button.dataset.status });
      await loadKeys();
    } catch (error) { setAdminStatus(error.message); }
    return;
  }

  if (action === "delete") {
    const key = row.querySelector(".key").textContent;
    if (!window.confirm(`Xóa vĩnh viễn key đã bị chặn?\n${key}\nDữ liệu session/báo cáo của key cũng sẽ bị xóa.`)) return;
    try {
      await api("/admin/deleteKey", { keyHash });
      setAdminStatus("Đã xóa key bị chặn.");
      await loadKeys();
    } catch (error) { setAdminStatus(error.message); }
    return;
  }

  if (action === "usage") {
    try {
      const key = row.querySelector(".key").textContent;
      const data = await api("/admin/usage", { keyHash });
      usageTitle.textContent = `Báo cáo: ${key}`;
      usageBody.innerHTML = (data.usage || []).length ? (data.usage || []).map((item) => `
        <div class="usageRow">
          <strong>${esc(item.date)}</strong>
          <span>${esc(item.launches)} lần mở</span>
          <span>${esc(formatSeconds(item.activeSeconds))}</span>
          <span>${esc(item.machineName || "-")} • v${esc(item.appVersion || "-")} • cuối ${esc(formatDate(item.lastSeenAt))}</span>
        </div>`).join("") : "<div class=\"muted\">Chưa có dữ liệu sử dụng.</div>";
      usageCard.classList.remove("hidden");
      usageCard.scrollIntoView({ behavior: "smooth", block: "start" });
    } catch (error) { setAdminStatus(error.message); }
  }
});

$("closeUsageBtn").addEventListener("click", () => usageCard.classList.add("hidden"));

if (supabase) {
  supabase.auth.onAuthStateChange((_event, _session) => {
    setTimeout(() => renderAuthState(), 0);
  });
  renderAuthState();
}
