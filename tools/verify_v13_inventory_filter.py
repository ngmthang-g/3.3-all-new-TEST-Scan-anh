from pathlib import Path
import hashlib, re, sys
ROOT=Path(__file__).resolve().parents[1]
C=(ROOT/'src/controller.cpp').read_text(encoding='utf-8')
B=(ROOT/'src/bridge.cpp').read_text(encoding='utf-8')
P=(ROOT/'src/protocol.h').read_text(encoding='utf-8')
L=(ROOT/'src/inventory_filter_logic.h').read_text(encoding='utf-8')
T=(ROOT/'src/inventory_filter_logic_test.cpp').read_text(encoding='utf-8')
CM=(ROOT/'CMakeLists.txt').read_text(encoding='utf-8')
V=(ROOT/'VERSION.txt').read_text(encoding='utf-8').strip()
errors=[]
def need(x,msg):
    if not x: errors.append(msg)
def function_body(text,sig):
    p=text.find(sig)
    if p<0:return None
    b=text.find('{',p)
    if b<0:return None
    d=0
    for i in range(b,len(text)):
        if text[i]=='{':d+=1
        elif text[i]=='}':
            d-=1
            if d==0:return text[p:i+1]
    return None

need(V=='1.3',f'VERSION must be 1.3, got {V!r}')
need('VERSION 1.3.0' in CM,'CMake version missing 1.3.0')
need('v1.3 • AUTO PK + LỌC ĐỒ + TELEGRAM' in C,'v1.3 title missing')
need('kProtocolVersion = 0x00010622u' in P,'protocol must be 0x00010622')
for marker in ['ReadBagPage = 19','DropBagItem = 20','SellBagItem = 21','struct BagItemSnapshot','struct BagPageSnapshot']:
    need(marker in P,'protocol inventory contract missing: '+marker)

# Unrelated core AUTO/AutoPK/travel/death behavior remains byte-identical to v1.2.
protected={
'    void TickAutoPk(':'1eb5447ae825ad9144c1cce2bbb770ca0c0bff04af99a6b02456ea4a36f29a48',
'    bool EnsureAutoFightOffForTravel(':'582e438d5d7a86e46deb8e3ae5d2a5ab64a66cf83a618a4dcf23ea26e740117e',
'    bool HandleRobustTravel(':'d360b7f77bfc666843987adcc74fe7df0728d42c037c8fa853ea8650d064b7ca',
'    bool PriorityReviveClick(':'934e7b2edf82c379eaec7d553e868ea16058a640e7183b179d93a3a5a94a2b4c',
'    bool PriorityAutoClick(':'fe754f63af9c1b967487519109b070b85c2049970b46aa8b85945948350b9439',
'    void ResetRuntimeForLifeBoundary(':'d6bebaa3eb4d107385c73cd34fcf230bb397fcb58f56485334fb067680aebdb7',
'    void BeginTrainRecovery(':'f23b32173caf1eb7776317144ec79632a81484f0ab400bc758165470382ec66f',
'    bool HandleDeath(':'21fb826e614cba4fa85a073b51a56d824263cf0ae310d1619bf77a3b99864642',
'    void AbortTrade(':'a2a88b182f99a97e4c6b9eb219aaf5701524c308650a223b00ccfcf65d74e899',
}
for sig,expected in protected.items():
    body=function_body(C,sig); need(body is not None,'missing protected '+sig.strip())
    if body is not None:
        got=hashlib.sha256(body.encode()).hexdigest(); need(got==expected,f'protected core changed {sig.strip()} -> {got}')

for marker in ['LỌC ĐỒ TAY NẢI','QUÉT LẠI TAY NẢI','+ GIỮ','+ VỨT/HỦY','+ BÁN','XÓA RULE',
               'DropEquipNonWeapon','SellEquipNonWeapon','DropCommon','SellCommon','DropGem','SellGem',
               'DropMedicine','SellMedicine','DropPetEquip','SellPetEquip']:
    need(marker in C,'filter UI/persistence missing: '+marker)
for marker in ['QuestProtected','protectBound','keepWeapons','IsItemThrowable','FindCandidate','HasSellRules']:
    need(marker in L or marker in C,'filter safety missing: '+marker)
need('if (TryAutoInventoryDrop(a, now)) return;' in C,'AUTO does not run drop filter before sell/trade')
need('a.runtime.bagFilterPendingInstance != 0' in C,'Trade gate does not block pending bag mutation')
need('SemanticSellRulesActive()' in C and 'Command::SellBagItem' in C,'semantic sell integration missing')
need('inventory_filter_logic_tests' in CM,'inventory filter tests missing from CMake')

for marker in ['GetItemsAtSite','GetItemType','GetEquipType','GetItemName','IsItemThrowable','IsItemSellable']:
    need(marker in B,'bridge semantic bag API missing: '+marker)
need('std::string("4:")' in B and '100005' in B,'exact abandon packet missing')
need('RequestSellItem' in B and 'NPCShop_SellItemTab' in B,'stock shop semantic sell call missing')
need('FindFreshBagItem' in B and 'current.site == 10' in B,'fresh live instance revalidation missing')

label=function_body(C,'    std::wstring TelegramAccountLabel(')
need(label is not None,'TelegramAccountLabel missing')
if label:
    for bad in ['PID ', 'RoleID ', 'a.displayName', 'a.snapshot.roleID']:
        need(bad not in label,'Telegram label leaks numeric identity: '+bad)
need('(L"PID " + std::to_wstring(kv.first))' not in C,'Telegram summary PID fallback still present')

# IDs remain disjoint with existing tabs.
pk=[int(x) for x in re.findall(r'constexpr int IDC_PK_[A-Z0-9_]+ = (\d+);',C)]
tg=[int(x) for x in re.findall(r'constexpr int IDC_TG_[A-Z0-9_]+ = (\d+);',C)]
inv=[int(x) for x in re.findall(r'constexpr int IDC_IF_[A-Z0-9_]+ = (\d+);',C)]
need(inv and all(600<=x<=629 for x in inv),'inventory window IDs escaped 600-629')
need(set(inv).isdisjoint(pk) and set(inv).isdisjoint(tg),'inventory IDs overlap AutoPK/Telegram')

if errors:
    for e in errors: print('FAIL:',e)
    sys.exit(1)
print('verify_v13_inventory_filter PASS')
