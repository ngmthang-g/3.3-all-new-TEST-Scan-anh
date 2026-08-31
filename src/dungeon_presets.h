#pragma once
#include "dungeon_logic.h"
#include <vector>
namespace cleanroute_dungeon {
inline Step M(const wchar_t*l,int map,int x,int y,int tol=120,
              std::uint32_t mask=kAllParticipantsMask,int parallel=0,bool fightOnArrival=false){
    Step s{};s.kind=StepKind::Move;s.label=l;s.mapID=map;s.x=x;s.y=y;s.tolerance=tol;s.timeoutSec=120;
    s.participantMask=mask;s.parallelGroup=parallel;s.autoFightOnArrival=fightOnArrival;return s;
}
inline Step F(const wchar_t*l,const wchar_t*n,int radius=800,bool boss=false){Step s{};s.kind=StepKind::Fight;s.label=l;s.radius=radius;s.monsterName=n?n:L"";s.group=boss?L"BOSS":L"THUONG";s.boss=boss;s.timeoutSec=boss?300:600;return s;}
inline Step W(const wchar_t*l,int sec){Step s{};s.kind=StepKind::Wait;s.label=l;s.delayMs=sec*1000;s.timeoutSec=sec+10;return s;}
inline Step P(const wchar_t*l,int map,int x,int y){Step s=M(l,map,x,y,100);s.kind=StepKind::Portal;s.timeoutSec=90;return s;}
inline void Pair(std::vector<Step>& v, const wchar_t* name, int map, int x, int y,
                 int radius = 800, bool boss = false) {
    v.push_back(M(name, map, x, y));
    Step fight = F(name, name, radius, boss);
    fight.mapID = map;
    fight.x = x;
    fight.y = y;
    v.push_back(std::move(fight));
}
inline Preset Base(const wchar_t*id,const wchar_t*name,int dm,int gm,int npc,int gx,int gy,const wchar_t*dialog,int minp=1){Preset p{};p.id=id;p.name=name;p.dungeonMap=dm;p.gatherMap=gm;p.npcResID=npc;p.gatherX=gx;p.gatherY=gy;p.dialogText=dialog;p.minPlayers=minp;return p;}
inline std::vector<Preset> CanonicalPresets(){
 std::vector<Preset> a;
 {auto p=Base(L"ThuyLao",L"Thủy Lao",92,4,650,10990,6466,L"Bình Định Thủy Lao Tạo Phản");
  const int x[]={2688,3808,1568,1312,1312,1280,2272,3424,4608,4768,4800,4832}; const int y[]={1312,1312,1280,2240,3424,4672,4800,4832,4832,3840,2720,1376};
  const wchar_t*b[]={L"Tiêu Vô Thường",L"Vân Trung Nhạn",L"Mãng Cái Thám Tử",L"Điền Doãn",L"Bách Hoa Sát",L"Vô Phương Hoà Thượng",L"Cách Liệt Mã",L"Hô Diên Bình",L"Diên Kiên",L"Hứa Đạo Nhân",L"Tiêu Vu Nhi",L"Tào Song"};
  for (int i = 0; i < 12; ++i) {
      p.steps.push_back(M((L"TỌA " + std::to_wstring(i + 1)).c_str(), 92, x[i], y[i]));
      Step normal = F(L"Phạm nhân bình thường", L"Phạm Nhân", 800);
      normal.mapID = 92; normal.x = x[i]; normal.y = y[i];
      p.steps.push_back(std::move(normal));
      Step boss = F(b[i], b[i], 800, true);
      boss.mapID = 92; boss.x = x[i]; boss.y = y[i];
      p.steps.push_back(std::move(boss));
  }
  a.push_back(std::move(p));}
 {auto p=Base(L"Q1_ToChau",L"Biên Giới Tống Liêu",93,4,674,4400,8090,L"Một tên cũng không thể thoát");
  constexpr std::uint32_t S12=(1u<<0)|(1u<<1);
  constexpr std::uint32_t S3456=(1u<<2)|(1u<<3)|(1u<<4)|(1u<<5);
  p.steps.push_back(M(L"HỘI QUÂN ĐẦU",93,1765,2665));
  p.steps.push_back(M(L"TÁCH 1-2 • điểm A",93,1110,2456,120,S12,1));
  p.steps.push_back(M(L"TÁCH 3-6 • điểm A",93,3072,2314,120,S3456,1));
  p.steps.push_back(M(L"TÁCH 1-2 • điểm B",93,1163,1759,120,S12,2));
  p.steps.push_back(M(L"TÁCH 3-6 • điểm B",93,2977,1656,120,S3456,2));
  p.steps.push_back(M(L"HỘI QUÂN ĐÁNH 1",93,2213,1665));
  {auto f=F(L"QUÉT SẠCH KHU HỘI QUÂN 1",L"",900);f.mapID=93;f.x=2213;f.y=1665;f.matchAnyVerified=true;p.steps.push_back(std::move(f));}
  p.steps.push_back(M(L"TÁCH 1-2 • đánh 10s",93,1279,1627,120,S12,3,true));
  p.steps.push_back(M(L"TÁCH 3-6 • đánh 10s",93,2503,1425,120,S3456,3,true));
  p.steps.push_back(W(L"ĐÁNH TÁCH 10 GIÂY",10));
  p.steps.push_back(M(L"HỘI QUÂN CUỐI",93,2071,1746));
  {auto f=F(L"QUÉT SẠCH KHU CUỐI",L"",900);f.mapID=93;f.x=2071;f.y=1746;f.matchAnyVerified=true;p.steps.push_back(std::move(f));}
  a.push_back(std::move(p));}
 {auto p=Base(L"Q2_ToChau",L"Trúc Lâm",94,4,707,11300,9890,L"Trừ hại");Pair(p.steps,L"Dã Hùng",94,2113,485);Pair(p.steps,L"Hồng Hùng Vương",94,1461,2970,800,true);a.push_back(std::move(p));}
 {auto p=Base(L"Q3_ToChau",L"Dã Ngoại Trại Phỉ",95,4,674,4500,8190,L"Doanh trại của phỉ");Pair(p.steps,L"Cát Vinh",95,1856,2208,800,true);a.push_back(std::move(p));}
 for(int mode=0;mode<3;++mode){const wchar_t*ids[]={L"DanhCo1",L"DanhCo2",L"DanhCo3"};const wchar_t*names[]={L"Trân Long Kỳ Cuộc (thường)",L"Trân Long Kỳ Cuộc (nhanh)",L"Trân Long Kỳ Cuộc (siêu tốc)"};auto p=Base(ids[mode],names[mode],77,4,717,8600,8600,names[mode]);p.steps.push_back(W(L"Chờ quân cờ",25));Pair(p.steps,L"Tiêu diệt quân cờ",77,1704,2503);p.steps.back().matchAnyVerified=true;Pair(p.steps,L"Viễn Cổ Kỳ Hồn",77,2016,2080,800,true);a.push_back(std::move(p));}
 for(int mode=0;mode<3;++mode){const wchar_t*ids[]={L"LauLanTamBao_1",L"LauLanTamBao_2",L"LauLanTamBao_3"};const wchar_t*names[]={L"Lâu Lan Tầm Bảo (thường)",L"Lâu Lan Tầm Bảo (nhanh)",L"Lâu Lan Tầm Bảo (siêu tốc)"};auto p=Base(ids[mode],names[mode],101,5,384,5200,7700,names[mode]);p.steps.push_back(W(L"Chờ bảo sương",25));Pair(p.steps,L"Tiêu diệt bảo sương",101,2016,1504);p.steps.back().matchAnyVerified=true;Pair(p.steps,L"Trấn Bảo Long Vương",101,2048,2048,800,true);a.push_back(std::move(p));}
 for(int mode=0;mode<3;++mode){const wchar_t*ids[]={L"TucCau_1",L"TucCau_2",L"TucCau_3"};const wchar_t*names[]={L"Túc Cầu (thường)",L"Túc Cầu (nhanh)",L"Túc Cầu (siêu tốc)"};std::wstring dialog=L"Tham gia "+std::wstring(names[mode]);auto p=Base(ids[mode],names[mode],103,4,718,9300,9170,dialog.c_str());p.steps.push_back(W(L"Chờ túc cầu",25));Pair(p.steps,L"Tiêu diệt túc cầu",103,1312,3072);p.steps.back().matchAnyVerified=true;Pair(p.steps,L"Tôn Mỹ Mỹ",103,1952,2272,800,true);a.push_back(std::move(p));}
 {auto p=Base(L"Q1_LauLan",L"Hoàng Kim Chi Liên",108,5,385,9400,8000,L"Hoàng Kim Chi Liên");Pair(p.steps,L"Huyền Lôi Pha Thổ Phỉ",108,4341,2303);Pair(p.steps,L"Ngưu Khúc",108,5376,2304,800,true);Pair(p.steps,L"Ngưu Kỳ",108,2080,7104,800,true);Pair(p.steps,L"Vương Diêm",108,6400,6656,800,true);a.push_back(std::move(p));}
 {auto p=Base(L"Q2_LauLan",L"Huyền Phật Châu",109,5,385,9400,8000,L"Huyền Phật Châu");Pair(p.steps,L"Độc Chướng Tiểu Quái",109,4224,1824);Pair(p.steps,L"Tê Phong Ma",109,4224,1920,800,true);Pair(p.steps,L"Liệt Địa Hành Giả",109,2656,5888,800,true);Pair(p.steps,L"Võ Huyền Tướng",109,5824,2880,800,true);Pair(p.steps,L"Ngũ Độc Ma Sứ",109,5792,5888,800,true);Pair(p.steps,L"Phá Diệm Tôn Giả",109,2304,3872,800,true);Pair(p.steps,L"Hồng Cúc Yêu Vương",109,4064,4416,800,true);a.push_back(std::move(p));}
 {auto p=Base(L"Q3_LauLan",L"Dung Nham Chi Địa",110,5,386,4000,8500,L"Dung Nham Chi Địa");Pair(p.steps,L"Yêu Ma Tùy Tùng",110,5984,2560);Pair(p.steps,L"Hỏa Diệm Yêu Ma",110,2144,6656,800,true);a.push_back(std::move(p));}
 {auto p=Base(L"SatTinh",L"Thập Nhị Sát Tinh",111,2,41,4170,7840,L"Khiêu chiến Thập Nhị Sát Tinh");p.steps.push_back(W(L"Chờ Sát Tinh",25));p.steps.push_back(M(L"Điểm Sát Tinh",111,2037,2550));auto f=F(L"Sát Tinh theo level-band",L"",1000,true);f.matchAnyVerified=true;p.steps.push_back(f);a.push_back(std::move(p));}
 {auto p=Base(L"PMVL",L"Yến Tử Ô",105,25,721,2200,6400,L"Thảo phạt Yến Tử Ô");p.steps.push_back(P(L"Cổng 1",105,5670,1090));Pair(p.steps,L"Đợt 1",105,2660,1520);p.steps.push_back(P(L"Cổng 2",105,2080,1420));Pair(p.steps,L"Đợt 2",105,7300,2400);p.steps.push_back(P(L"Cổng 3",105,5830,5400));Pair(p.steps,L"Đợt 3",105,4800,5500);Pair(p.steps,L"Đợt 4",105,2000,4600);Pair(p.steps,L"Đợt 5",105,1900,6200);for(auto& s:p.steps) if(s.kind==StepKind::Fight&&s.monsterName.find(L"Đợt")==0) s.matchAnyVerified=true;a.push_back(std::move(p));}
 {auto p=Base(L"PhiKH",L"Diệt Phỉ Kính Hồ",107,30,301,6400,8430,L"Diệt phỉ Kính Hồ");p.steps.push_back(W(L"Chờ thủy phỉ",25));Pair(p.steps,L"Kính Hồ Thuỷ Phỉ",107,992,864);Pair(p.steps,L"Kính Hồ Thủy Phỉ Đầu Lĩnh",107,4384,4832,800,true);Pair(p.steps,L"Kính Hồ Thủy Phỉ Đầu Lĩnh",107,4448,4800,800,true);a.push_back(std::move(p));}
 return a;
}
}
