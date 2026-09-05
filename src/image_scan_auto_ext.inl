#include <algorithm>
#include <map>
#include <utility>

namespace image_scan_test {
namespace {

enum class AutoPhase {
    Idle,
    WaitOpen1,
    WaitOpen2,
    WaitItemReady,
    WaitDiscardGone,
    WaitPopupGoneAfterConfirm,
    WaitCloseGone,
    WaitEmptyRetry,
    WaitBagClose,
    CompletedUntilFull,
    FullYieldReady,
    Error,
};

enum class AutoClosePurpose { None, Full, Completed };

struct AutoKey {
    HWND hwnd = nullptr;
    int childSlot = 0;
    bool operator<(const AutoKey& other) const {
        if (hwnd != other.hwnd) return hwnd < other.hwnd;
        return childSlot < other.childSlot;
    }
};

struct AutoSession {
    State scan{}; // reuse FILTER V4 Config/Image/Target helpers in this translation unit.
    AutoPhase phase = AutoPhase::Idle;
    AutoClosePurpose closePurpose = AutoClosePurpose::None;
    bool initialized = false;
    bool currentItemActive = false;
    bool fullExitPending = false;
    bool bagOpened = false;
    std::size_t slotIndex = 0;
    std::size_t discardCount = 0;
    ULONGLONG nextTick = 0;
    ULONGLONG deadlineTick = 0;
    std::wstring status{};
};

std::map<AutoKey, AutoSession> g_autoSessions;

bool ValidChildSlot(int childSlot) { return childSlot >= 1 && childSlot <= 12; }

AutoSession* FindAutoSession(HWND hwnd, int childSlot, bool create) {
    if (!hwnd || !ValidChildSlot(childSlot)) return nullptr;
    const AutoKey key{hwnd, childSlot};
    auto it = g_autoSessions.find(key);
    if (it != g_autoSessions.end()) return &it->second;
    if (!create) return nullptr;
    return &g_autoSessions.emplace(key, AutoSession{}).first->second;
}

void ClearAutoSessionRuntime(AutoSession& s) {
    s.scan = State{};
    s.phase = AutoPhase::Idle;
    s.closePurpose = AutoClosePurpose::None;
    s.initialized = false;
    s.currentItemActive = false;
    s.fullExitPending = false;
    s.bagOpened = false;
    s.slotIndex = 0;
    s.discardCount = 0;
    s.nextTick = 0;
    s.deadlineTick = 0;
    s.status.clear();
}

bool AutoClickStep(AutoSession& s, const ClickStep& step, std::wstring& error) {
    int cw=0,ch=0,x=0,y=0;
    if(!CurrentClientSize(s.scan.target.gameWindow,cw,ch)||!ResolveStepPoint(step,cw,ch,x,y)){
        error=L"không resolve được tọa click";return false;
    }
    return RawClick(s.scan,x,y,cw,ch,error);
}

bool AutoValidateAndLoad(AutoSession& s, std::wstring& error) {
    EnsurePersistentConfigLoaded();
    s.scan.config = g_lastConfig;
    RebaseForCurrentClient(s.scan.config, s.scan.target.gameWindow);
    if (s.scan.config.templatePath.empty()) { error=L"chưa chọn ẢNH 1"; return false; }
    if (s.scan.config.discardTemplatePath.empty()) { error=L"chưa chọn ảnh VỨT"; return false; }
    if (s.scan.config.closeTemplatePath.empty()) { error=L"chưa chọn ảnh X"; return false; }
    if (s.scan.config.steps.empty()) { error=L"AUTO chưa có tọa scan"; return false; }
    for (std::size_t i=0;i<s.scan.config.steps.size();++i) {
        const ClickStep& step=s.scan.config.steps[i];
        if(!step.valid||step.baseW<=0||step.baseH<=0){error=L"CLICK "+std::to_wstring(i+1)+L" chưa gán F8";return false;}
    }
    if(!s.scan.config.afterDiscard.valid){error=L"chưa gán CLICK SAU VỨT";return false;}
    if(!s.scan.config.closeBag.valid){error=L"chưa gán CLOSE BAG";return false;}
    if(!LoadImageWic(s.scan.config.templatePath,s.scan.goodTpl,error)){error=L"ẢNH 1 • "+error;return false;}
    if(!LoadImageWic(s.scan.config.discardTemplatePath,s.scan.discardTpl,error)){error=L"ẢNH VỨT • "+error;return false;}
    if(!LoadImageWic(s.scan.config.closeTemplatePath,s.scan.closeTpl,error)){error=L"ẢNH X • "+error;return false;}
    return true;
}

void AutoArm(AutoSession& s, AutoPhase phase, ULONGLONG now, ULONGLONG delayMs, bool withDeadline=true) {
    s.phase=phase;s.nextTick=now+delayMs;s.deadlineTick=withDeadline?now+kStateTimeoutMs:0;
}

bool AutoTimedOut(const AutoSession& s, ULONGLONG now) {
    return s.deadlineTick != 0 && now >= s.deadlineTick;
}

void AutoSetError(AutoSession& s, const std::wstring& error) {
    s.phase=AutoPhase::Error;s.currentItemActive=false;
    s.status=L"SCAN VK ERROR • "+error;
}

bool AutoStartSlot(AutoSession& s, ULONGLONG now, std::wstring& error) {
    if (s.slotIndex >= s.scan.config.steps.size()) { error=L"slot vượt danh sách scan"; return false; }
    const ClickStep& step=s.scan.config.steps[s.slotIndex];
    if(!AutoClickStep(s,step,error))return false;
    s.currentItemActive=true;
    AutoArm(s,AutoPhase::WaitItemReady,now,static_cast<ULONGLONG>(std::clamp(step.delayMs,50,10000)));
    s.status=L"SCAN VK • CLICK "+std::to_wstring(s.slotIndex+1)+L" → chờ ảnh";
    return true;
}

bool AutoStartClose(AutoSession& s, ULONGLONG now, AutoClosePurpose purpose, std::wstring& error) {
    if(!AutoClickStep(s,s.scan.config.closeBag,error))return false;
    s.bagOpened=false;s.currentItemActive=false;s.closePurpose=purpose;
    AutoArm(s,AutoPhase::WaitBagClose,now,static_cast<ULONGLONG>(std::clamp(s.scan.config.closeBag.delayMs,50,10000)),false);
    s.status=purpose==AutoClosePurpose::Full
        ? L"SCAN VK • FULL → CLOSE BAG → nhường điều phối"
        : L"SCAN VK • SLOT CUỐI GOOD → CLOSE BAG → chờ FULL";
    return true;
}

bool AutoOpenNext(AutoSession& s, ULONGLONG now, int stage, std::wstring& error) {
    const ClickStep* point = stage==1 ? &s.scan.config.openBag1 : &s.scan.config.openBag2;
    if(point->valid){
        if(!AutoClickStep(s,*point,error))return false;
        s.bagOpened=true;
        AutoArm(s,stage==1?AutoPhase::WaitOpen1:AutoPhase::WaitOpen2,now,
                static_cast<ULONGLONG>(std::clamp(point->delayMs,50,10000)),false);
        s.status=L"SCAN VK • OPEN "+std::to_wstring(stage)+L" PASS";
        return true;
    }
    if(stage==1)return AutoOpenNext(s,now,2,error);
    s.bagOpened=true;
    return AutoStartSlot(s,now,error);
}

bool InitializeAutoSession(AutoSession& s, const Target& target, ULONGLONG now, std::wstring& error) {
    ClearAutoSessionRuntime(s);
    s.scan.target=target;
    if(!AutoValidateAndLoad(s,error)){AutoSetError(s,error);return false;}
    s.initialized=true;s.slotIndex=0;s.discardCount=0;s.fullExitPending=false;
    if(!AutoOpenNext(s,now,1,error)){AutoSetError(s,error);return false;}
    return true;
}

AutoFilterResult MakeAutoResult(const AutoSession& s) {
    AutoFilterResult r{};r.slotNumber=s.slotIndex<s.scan.config.steps.size()?s.slotIndex+1:0;r.status=s.status;
    switch(s.phase){
        case AutoPhase::WaitEmptyRetry:r.state=AutoFilterState::WaitingEmpty;r.ownsInput=true;r.fullYieldReady=false;break;
        case AutoPhase::CompletedUntilFull:r.state=AutoFilterState::CompletedUntilFull;r.fullYieldReady=true;break;
        case AutoPhase::FullYieldReady:r.state=AutoFilterState::FullYieldReady;r.fullYieldReady=true;break;
        case AutoPhase::Error:r.state=AutoFilterState::Error;r.fullYieldReady=true;break;
        case AutoPhase::Idle:r.state=AutoFilterState::Idle;r.fullYieldReady=true;break;
        default:r.state=AutoFilterState::Busy;r.ownsInput=true;r.fullYieldReady=false;break;
    }
    return r;
}

} // namespace

namespace {
bool g_persistentConfigLoadAttempted = false;

std::filesystem::path PersistentScanConfigPath() {
    wchar_t localAppData[4096]{};
    const DWORD n=GetEnvironmentVariableW(L"LOCALAPPDATA",localAppData,_countof(localAppData));
    if(n>0&&n<_countof(localAppData)){
        const std::filesystem::path dir=std::filesystem::path(localAppData)/L"ThanLongCleanRoute";
        std::error_code ec;std::filesystem::create_directories(dir,ec);
        return dir/L"WeaponScan.auto.tlscan";
    }
    wchar_t exe[MAX_PATH*4]{};GetModuleFileNameW(nullptr,exe,_countof(exe));
    return std::filesystem::path(exe).parent_path()/L"WeaponScan.auto.tlscan";
}

void SavePersistentConfigImpl() {
    const auto path=PersistentScanConfigPath();std::ofstream out(path,std::ios::binary|std::ios::trunc);if(!out)return;
    const Config& c=g_lastConfig;
    out<<"TL_SCAN_AUTO_V1\n";
    out<<"good_path="<<Utf8(c.templatePath)<<"\n"<<"discard_path="<<Utf8(c.discardTemplatePath)<<"\n"<<"close_path="<<Utf8(c.closeTemplatePath)<<"\n";
    out<<"good_roi="<<RoiLine(c.x,c.y,c.w,c.h,c.roiBaseW,c.roiBaseH)<<"\n";
    out<<"discard_roi="<<RoiLine(c.discardRoi.x,c.discardRoi.y,c.discardRoi.w,c.discardRoi.h,c.discardRoi.baseW,c.discardRoi.baseH)<<"\n";
    out<<"close_roi="<<RoiLine(c.closeRoi.x,c.closeRoi.y,c.closeRoi.w,c.closeRoi.h,c.closeRoi.baseW,c.closeRoi.baseH)<<"\n";
    out<<"threshold="<<c.thresholdPercent<<"\n"<<"delay_discard="<<c.discardClickDelayMs<<"\n"<<"delay_after="<<c.afterDiscardClickDelayMs<<"\n"<<"delay_x="<<c.closeClickDelayMs<<"\n";
    out<<"after_discard="<<StepLine(c.afterDiscard)<<"\n"<<"open1="<<StepLine(c.openBag1)<<"\n"<<"open2="<<StepLine(c.openBag2)<<"\n"<<"close_bag="<<StepLine(c.closeBag)<<"\n";
    out<<"children=";for(int i=0;i<12;++i){if(i)out<<',';out<<(c.childEnabled[static_cast<std::size_t>(i)]?1:0);}out<<"\n";
    out<<"step_count="<<c.steps.size()<<"\n";for(std::size_t i=0;i<c.steps.size();++i)out<<"step"<<i<<"="<<StepLine(c.steps[i])<<"\n";
}

void LoadPersistentConfigImpl() {
    std::ifstream in(PersistentScanConfigPath(),std::ios::binary);if(!in)return;
    std::string line;if(!std::getline(in,line)||line!="TL_SCAN_AUTO_V1")return;
    Config c{};std::size_t stepCount=0;std::vector<std::pair<std::size_t,std::string>> pendingSteps;
    while(std::getline(in,line)){
        if(!line.empty()&&line.back()=='\r')line.pop_back();const auto pos=line.find('=');if(pos==std::string::npos)continue;const std::string key=line.substr(0,pos),val=line.substr(pos+1);
        if(key=="good_path")c.templatePath=FromUtf8(val);else if(key=="discard_path")c.discardTemplatePath=FromUtf8(val);else if(key=="close_path")c.closeTemplatePath=FromUtf8(val);
        else if(key=="good_roi"){std::array<int,6>v{};if(ParseSix(val,v)){c.x=v[0];c.y=v[1];c.w=v[2];c.h=v[3];c.roiBaseW=v[4];c.roiBaseH=v[5];}}
        else if(key=="discard_roi"){std::array<int,6>v{};if(ParseSix(val,v))c.discardRoi={v[0],v[1],v[2],v[3],v[4],v[5]};}
        else if(key=="close_roi"){std::array<int,6>v{};if(ParseSix(val,v))c.closeRoi={v[0],v[1],v[2],v[3],v[4],v[5]};}
        else if(key=="threshold"){try{c.thresholdPercent=std::clamp(std::stoi(val),1,100);}catch(...){} }
        else if(key=="delay_discard"){try{c.discardClickDelayMs=std::clamp(std::stoi(val),50,10000);}catch(...){} }
        else if(key=="delay_after"){try{c.afterDiscardClickDelayMs=std::clamp(std::stoi(val),50,10000);}catch(...){} }
        else if(key=="delay_x"){try{c.closeClickDelayMs=std::clamp(std::stoi(val),50,10000);}catch(...){} }
        else if(key=="after_discard")ParseStepLine(val,c.afterDiscard);else if(key=="open1")ParseStepLine(val,c.openBag1);else if(key=="open2")ParseStepLine(val,c.openBag2);else if(key=="close_bag")ParseStepLine(val,c.closeBag);
        else if(key=="children"){std::stringstream ss(val);std::string x;int i=0;while(std::getline(ss,x,',')&&i<12)c.childEnabled[static_cast<std::size_t>(i++)]=(x=="1");}
        else if(key=="step_count"){try{stepCount=static_cast<std::size_t>(std::max(0,std::stoi(val)));}catch(...){} }
        else if(key.rfind("step",0)==0){try{pendingSteps.emplace_back(static_cast<std::size_t>(std::stoul(key.substr(4))),val);}catch(...){} }
    }
    c.steps.resize(stepCount);for(const auto& it:pendingSteps)if(it.first<c.steps.size())ParseStepLine(it.second,c.steps[it.first]);
    g_lastConfig=c;
}
} // namespace

void EnsurePersistentConfigLoaded() {
    if(g_persistentConfigLoadAttempted)return;g_persistentConfigLoadAttempted=true;LoadPersistentConfigImpl();
}

void SavePersistentConfig() {
    g_persistentConfigLoadAttempted=true;SavePersistentConfigImpl();
}

bool IsChildAutoFilterEnabled(int childSlot) {
    EnsurePersistentConfigLoaded();
    return ValidChildSlot(childSlot) && g_lastConfig.childEnabled[static_cast<std::size_t>(childSlot-1)];
}

AutoFilterResult TickAutoFilter(const Target& target, int childSlot, bool bagFull) {
    AutoFilterResult disabled{};disabled.state=AutoFilterState::Disabled;disabled.fullYieldReady=true;
    if(!IsChildAutoFilterEnabled(childSlot)||!target.gameWindow||!IsWindow(target.gameWindow)||!target.hiddenClick)return disabled;
    AutoSession* ps=FindAutoSession(target.gameWindow,childSlot,true);if(!ps)return disabled;AutoSession& s=*ps;s.scan.target=target;
    const ULONGLONG now=GetTickCount64();

    if(!s.initialized&&s.phase!=AutoPhase::Error){
        if(bagFull){s.fullExitPending=true;s.phase=AutoPhase::FullYieldReady;s.status=L"SCAN VK • FULL trước khi scan → nhường điều phối";return MakeAutoResult(s);}
        std::wstring error;if(!InitializeAutoSession(s,target,now,error))return MakeAutoResult(s);
    }
    if(s.phase==AutoPhase::Error)return MakeAutoResult(s);
    if(s.phase==AutoPhase::CompletedUntilFull){
        if(bagFull){s.fullExitPending=true;s.phase=AutoPhase::FullYieldReady;s.status=L"SCAN VK • SLOT CUỐI đã GOOD • FULL → về GD";}
        return MakeAutoResult(s);
    }
    if(s.phase==AutoPhase::FullYieldReady)return MakeAutoResult(s);

    if(bagFull)s.fullExitPending=true;
    std::wstring error;
    // FULL while no item transaction is active must close the bag immediately;
    // do not sit in the 10-second EMPTY wait or OpenBag delay before yielding.
    if(s.fullExitPending&&!s.currentItemActive&&
       (s.phase==AutoPhase::WaitOpen1||s.phase==AutoPhase::WaitOpen2||s.phase==AutoPhase::WaitEmptyRetry)){
        if(!AutoStartClose(s,now,AutoClosePurpose::Full,error))AutoSetError(s,error);
        return MakeAutoResult(s);
    }
    if(now<s.nextTick)return MakeAutoResult(s);

    if(s.phase==AutoPhase::WaitOpen1){
        if(s.fullExitPending){if(!AutoStartClose(s,now,AutoClosePurpose::Full,error))AutoSetError(s,error);return MakeAutoResult(s);}
        if(!AutoOpenNext(s,now,2,error))AutoSetError(s,error);return MakeAutoResult(s);
    }
    if(s.phase==AutoPhase::WaitOpen2){
        if(s.fullExitPending){if(!AutoStartClose(s,now,AutoClosePurpose::Full,error))AutoSetError(s,error);return MakeAutoResult(s);}
        if(!AutoStartSlot(s,now,error))AutoSetError(s,error);return MakeAutoResult(s);
    }
    if(s.phase==AutoPhase::WaitEmptyRetry){
        if(s.fullExitPending){if(!AutoStartClose(s,now,AutoClosePurpose::Full,error))AutoSetError(s,error);return MakeAutoResult(s);}
        if(!AutoStartSlot(s,now,error))AutoSetError(s,error);return MakeAutoResult(s);
    }
    if(s.phase==AutoPhase::WaitBagClose){
        if(s.closePurpose==AutoClosePurpose::Full){s.phase=AutoPhase::FullYieldReady;s.status=L"SCAN VK • CLOSE BAG xong • FULL đã nhường điều phối";}
        else{s.phase=AutoPhase::CompletedUntilFull;s.status=L"SCAN VK • SLOT CUỐI GOOD • dừng scan, chờ FULL";}
        s.closePurpose=AutoClosePurpose::None;return MakeAutoResult(s);
    }

    Image frame{};std::wstring backend;
    if(!CaptureClient(target.gameWindow,frame,backend,error)){AutoSetError(s,L"CAPTURE • "+error);return MakeAutoResult(s);}

    if(s.phase==AutoPhase::WaitItemReady){
        Match good{};if(!ScanGoodOnFrame(s.scan,frame,good,error)){AutoSetError(s,L"ROI GOOD • "+error);return MakeAutoResult(s);}
        if(good.found){
            Match close{};if(!ScanCloseOnFrame(s.scan,frame,close,error)){AutoSetError(s,L"ROI X • "+error);return MakeAutoResult(s);}
            if(close.found){
                const int cx=close.x+s.scan.closeTpl.width/2,cy=close.y+s.scan.closeTpl.height/2;
                if(!RawClick(s.scan,cx,cy,frame.width,frame.height,error)){AutoSetError(s,L"CLICK X • "+error);return MakeAutoResult(s);}
                AutoArm(s,AutoPhase::WaitCloseGone,now,static_cast<ULONGLONG>(std::clamp(s.scan.config.closeClickDelayMs,50,10000)));
                s.status=L"SCAN VK • SLOT "+std::to_wstring(s.slotIndex+1)+L" GOOD → X";return MakeAutoResult(s);
            }
            if(AutoTimedOut(s,now))AutoSetError(s,L"GOOD nhưng X không xuất hiện tại slot "+std::to_wstring(s.slotIndex+1));
            else s.nextTick=now+kProbeIntervalMs;
            return MakeAutoResult(s);
        }
        Match discard{};if(!ScanDiscardOnFrame(s.scan,frame,discard,error)){AutoSetError(s,L"ROI VỨT • "+error);return MakeAutoResult(s);}
        if(discard.found){
            const int dx=discard.x+s.scan.discardTpl.width/2,dy=discard.y+s.scan.discardTpl.height/2;
            if(!RawClick(s.scan,dx,dy,frame.width,frame.height,error)){AutoSetError(s,L"CLICK VỨT • "+error);return MakeAutoResult(s);}
            AutoArm(s,AutoPhase::WaitDiscardGone,now,static_cast<ULONGLONG>(std::clamp(s.scan.config.discardClickDelayMs,50,10000)));
            s.status=L"SCAN VK • SLOT "+std::to_wstring(s.slotIndex+1)+L" BAD → VỨT";return MakeAutoResult(s);
        }
        s.currentItemActive=false;
        if(s.fullExitPending){if(!AutoStartClose(s,now,AutoClosePurpose::Full,error))AutoSetError(s,error);return MakeAutoResult(s);}
        s.phase=AutoPhase::WaitEmptyRetry;s.nextTick=now+kEmptyRetryMs;s.deadlineTick=0;
        s.status=L"SCAN VK • SLOT "+std::to_wstring(s.slotIndex+1)+L" EMPTY → chờ 10s SAME SLOT";return MakeAutoResult(s);
    }

    if(s.phase==AutoPhase::WaitDiscardGone){
        Match discard{};if(!ScanDiscardOnFrame(s.scan,frame,discard,error)){AutoSetError(s,L"ROI VỨT • "+error);return MakeAutoResult(s);}
        if(!discard.found){
            if(!AutoClickStep(s,s.scan.config.afterDiscard,error)){AutoSetError(s,L"CLICK SAU VỨT • "+error);return MakeAutoResult(s);}
            AutoArm(s,AutoPhase::WaitPopupGoneAfterConfirm,now,static_cast<ULONGLONG>(std::clamp(s.scan.config.afterDiscardClickDelayMs,50,10000)));
            s.status=L"SCAN VK • VỨT mất → xác nhận";return MakeAutoResult(s);
        }
        if(AutoTimedOut(s,now))AutoSetError(s,L"VỨT không biến mất");else s.nextTick=now+kProbeIntervalMs;return MakeAutoResult(s);
    }

    if(s.phase==AutoPhase::WaitPopupGoneAfterConfirm){
        Match good{},discard{},close{};std::wstring e1,e2,e3;
        const bool ok1=ScanGoodOnFrame(s.scan,frame,good,e1),ok2=ScanDiscardOnFrame(s.scan,frame,discard,e2),ok3=ScanCloseOnFrame(s.scan,frame,close,e3);
        if(!ok1||!ok2||!ok3){AutoSetError(s,L"SCAN sau VỨT • "+(!ok1?e1:(!ok2?e2:e3)));return MakeAutoResult(s);}
        if(!good.found&&!discard.found&&!close.found){
            ++s.discardCount;s.currentItemActive=false;
            if(s.fullExitPending){if(!AutoStartClose(s,now,AutoClosePurpose::Full,error))AutoSetError(s,error);return MakeAutoResult(s);}
            if(!AutoStartSlot(s,now,error))AutoSetError(s,L"LẶP SAME SLOT • "+error);else s.status=L"SCAN VK • đã vứt → CLICK LẠI SAME SLOT "+std::to_wstring(s.slotIndex+1);
            return MakeAutoResult(s);
        }
        if(AutoTimedOut(s,now))AutoSetError(s,L"popup sau VỨT chưa đóng");else s.nextTick=now+kProbeIntervalMs;return MakeAutoResult(s);
    }

    if(s.phase==AutoPhase::WaitCloseGone){
        Match close{};if(!ScanCloseOnFrame(s.scan,frame,close,error)){AutoSetError(s,L"ROI X • "+error);return MakeAutoResult(s);}
        if(!close.found){
            s.currentItemActive=false;s.discardCount=0;
            if(s.fullExitPending){if(!AutoStartClose(s,now,AutoClosePurpose::Full,error))AutoSetError(s,error);return MakeAutoResult(s);}
            if(s.slotIndex+1>=s.scan.config.steps.size()){if(!AutoStartClose(s,now,AutoClosePurpose::Completed,error))AutoSetError(s,error);return MakeAutoResult(s);}
            ++s.slotIndex;if(!AutoStartSlot(s,now,error))AutoSetError(s,error);return MakeAutoResult(s);
        }
        if(AutoTimedOut(s,now))AutoSetError(s,L"X không biến mất");else s.nextTick=now+kProbeIntervalMs;return MakeAutoResult(s);
    }

    return MakeAutoResult(s);
}

bool FullBagTravelLatched(HWND gameWindow, int childSlot) {
    if(!IsChildAutoFilterEnabled(childSlot))return false;
    AutoSession* s=FindAutoSession(gameWindow,childSlot,false);
    return s && (s->fullExitPending || s->phase==AutoPhase::FullYieldReady);
}

bool FullBagYieldReady(HWND gameWindow, int childSlot) {
    if(!IsChildAutoFilterEnabled(childSlot))return true;
    AutoSession* s=FindAutoSession(gameWindow,childSlot,false);if(!s)return true;
    switch(s->phase){
        case AutoPhase::FullYieldReady:
        case AutoPhase::CompletedUntilFull:
        case AutoPhase::Error:
        case AutoPhase::Idle:return true;
        default:return false;
    }
}

void NotifyDeath(HWND gameWindow, int childSlot) {
    AutoSession* s=FindAutoSession(gameWindow,childSlot,false);if(!s)return;
    s->phase=AutoPhase::Idle;s->initialized=false;s->currentItemActive=false;s->fullExitPending=false;
    s->status=L"SCAN VK • DEATH → nhường ĐẦU THAI";
}

void NotifyReviveClicked(const Target& target, int childSlot) {
    if(!IsChildAutoFilterEnabled(childSlot)||!target.gameWindow||!target.hiddenClick)return;
    AutoSession* s=FindAutoSession(target.gameWindow,childSlot,true);if(!s)return;s->scan.target=target;
    Config cfg=g_lastConfig;RebaseForCurrentClient(cfg,target.gameWindow);std::wstring error;
    if(cfg.closeBag.valid){s->scan.config=cfg;(void)AutoClickStep(*s,cfg.closeBag,error);}
    ClearAutoSessionRuntime(*s);s->status=error.empty()?L"SCAN VK • sau ĐẦU THAI đã click CLOSE BAG":L"SCAN VK • CLOSE BAG sau ĐẦU THAI fail • "+error;
}

void ResetAutoFilter(HWND gameWindow, int childSlot) {
    AutoSession* s=FindAutoSession(gameWindow,childSlot,false);if(!s)return;ClearAutoSessionRuntime(*s);
}

void StopAutoFilter(HWND gameWindow, int childSlot) {
    const AutoKey key{gameWindow,childSlot};g_autoSessions.erase(key);
}

} // namespace image_scan_test
