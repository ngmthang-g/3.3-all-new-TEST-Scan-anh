from pathlib import Path
import hashlib, re, sys
ROOT=Path(__file__).resolve().parents[1]
C=(ROOT/'src/controller.cpp').read_text(encoding='utf-8')
B=(ROOT/'src/bridge.cpp').read_text(encoding='utf-8')
P=(ROOT/'src/protocol.h').read_text(encoding='utf-8')
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

need(V=='1.4',f'VERSION must be 1.4, got {V!r}')
need('VERSION 1.4.0' in CM,'CMake version missing 1.4.0')
need('v1.4 • AUTO PK + LỌC ĐỒ + TELEGRAM' in C,'v1.4 title missing')
need('kProtocolVersion = 0x00010622u' in P,'protocol must remain 0x00010622')

# v1.3 inventory contract/actions stay intact.
for marker in ['ReadBagPage = 19','DropBagItem = 20','SellBagItem = 21','struct BagItemSnapshot','struct BagPageSnapshot']:
    need(marker in P,'inventory protocol contract missing: '+marker)
for marker in ['GetItemsAtSite','GetItemAtSite','GetItemType','GetEquipType','GetItemName','IsItemThrowable','IsItemSellable']:
    need(marker in B,'bridge semantic bag API missing: '+marker)
need('std::string("4:")' in B and '100005' in B,'exact abandon packet missing')
need('RequestSellItem' in B and 'NPCShop_SellItemTab' in B,'stock shop semantic sell call missing')
need('FindFreshBagItem' in B and 'current.site == 10' in B,'fresh live instance revalidation missing')

# v1.4 sparse/keyed collection hotfix: never fail whole scan on first object indexer exception.
for marker in ['TryReadBagIndexed','TryReadBagEnumerator','TryUnwrapBagEnumeratorCurrent','TryReadBagByPosition','BagItemAtSiteMethod']:
    need(marker in B,'v1.4 bag fallback missing: '+marker)
need('if (!readOk) readOk = TryReadBagEnumerator(collection, cc, items);' in B,
     'enumerator fallback not chained after indexed path')
need('if (!readOk) readOk = TryReadBagByPosition(c, items);' in B,
     'position fallback not chained after enumerator path')
need('FindMethod(cc, "GetEnumerator", 0)' in B and 'FindMethod(ec, "MoveNext", 0)' in B and 'FindMethod(ec, "get_Current", 0)' in B,
     'managed enumerator surface incomplete')
need('FindMethod(klass, "get_Value", 0)' in B,
     'KeyValuePair.Value unwrap missing')
need('for (std::int32_t position = 0; position <= 100; ++position)' in B,
     '0/1-based 100-slot semantic fallback missing')
need('if (!getItem) getItem = FindMethod(cc, "get_Item", 1);' not in B,
     'unsafe blind one-argument get_Item fallback still present')
need('if (!InvokeObjectArgs(getItem, collection, itemArgs, item, itemError, _countof(itemError)))\n            return false;' in B,
     'indexed managed exception is not fail-soft inside strategy')
need('Không enumerate được tay nải (indexed/enumerator/position đều fail)' in B,
     'final bag scan diagnostic missing')

# Unrelated core AUTO/AutoPK/travel/death behavior remains byte-identical.
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

# UI/filter and Telegram identity cleanup remain present.
for marker in ['LỌC ĐỒ TAY NẢI','QUÉT LẠI TAY NẢI','+ GIỮ','+ VỨT/HỦY','+ BÁN','XÓA RULE','SemanticSellRulesActive()']:
    need(marker in C,'v1.3 filter behavior regressed: '+marker)
label=function_body(C,'    std::wstring TelegramAccountLabel(')
need(label is not None,'TelegramAccountLabel missing')
if label:
    for bad in ['PID ', 'RoleID ', 'a.displayName', 'a.snapshot.roleID']:
        need(bad not in label,'Telegram label leaks numeric identity: '+bad)

if errors:
    for e in errors: print('FAIL:',e)
    sys.exit(1)
print('verify_v14_bag_scan PASS')
