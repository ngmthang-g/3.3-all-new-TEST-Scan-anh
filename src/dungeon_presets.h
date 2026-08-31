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
 {auto p=Base(L"Q2_ToChau",L"Trúc Lâm",94,4,707,11300,9890,L"Trừ hại");
  const int x[]={1130,1793,3567,3349,2168,822}; const int y[]={659,1128,1384,2099,2005,1882};
  for(int i=0;i<6;++i){p.steps.push_back(M((L"TRÚC LÂM • KHU "+std::to_wstring(i+1)).c_str(),94,x[i],y[i]));
    auto f=F((L"QUÉT SẠCH KHU "+std::to_wstring(i+1)).c_str(),L"",900);f.mapID=94;f.x=x[i];f.y=y[i];f.matchAnyVerified=true;p.steps.push_back(std::move(f));}
  p.steps.push_back(M(L"TRÚC LÂM • MOVE ONLY",94,3279,2774));
  p.steps.push_back(M(L"TRÚC LÂM • KHU CUỐI",94,1255,2791));
  {auto f=F(L"QUÉT SẠCH KHU CUỐI",L"",900);f.mapID=94;f.x=1255;f.y=2791;f.matchAnyVerified=true;p.steps.push_back(std::move(f));}
  a.push_back(std::move(p));}
 {auto p=Base(L"Q3_ToChau",L"Dã Ngoại Trại Phỉ",95,4,674,4500,8190,L"Doanh trại của phỉ");
  constexpr std::uint32_t S12=(1u<<0)|(1u<<1); constexpr std::uint32_t S3456=(1u<<2)|(1u<<3)|(1u<<4)|(1u<<5);
  p.steps.push_back(M(L"DÃ NGOẠI P1 • 1-2",95,1035,2145,120,S12,11));
  p.steps.push_back(M(L"DÃ NGOẠI P1 • 3-6",95,2112,3040,120,S3456,11));
  p.steps.push_back(M(L"DÃ NGOẠI P2 • 1-2",95,1444,1393,120,S12,12));
  p.steps.push_back(M(L"DÃ NGOẠI P2 • 3-6",95,2843,2493,120,S3456,12));
  p.steps.push_back(M(L"DÃ NGOẠI • HỘI QUÂN CUỐI",95,2039,2099));
  {auto f=F(L"DÃ NGOẠI • QUÉT SẠCH CUỐI",L"",1000);f.mapID=95;f.x=2039;f.y=2099;f.matchAnyVerified=true;p.steps.push_back(std::move(f));}
  a.push_back(std::move(p));}
 for(int mode=0;mode<3;++mode){const wchar_t*ids[]={L"DanhCo1",L"DanhCo2",L"DanhCo3"};const wchar_t*names[]={L"Trân Long Kỳ Cuộc (thường)",L"Trân Long Kỳ Cuộc (nhanh)",L"Trân Long Kỳ Cuộc (siêu tốc)"};auto p=Base(ids[mode],names[mode],77,4,717,8600,8600,names[mode]);p.steps.push_back(W(L"Chờ quân cờ",25));Pair(p.steps,L"Tiêu diệt quân cờ",77,1704,2503);p.steps.back().matchAnyVerified=true;Pair(p.steps,L"Viễn Cổ Kỳ Hồn",77,2016,2080,800,true);a.push_back(std::move(p));}
 for(int mode=0;mode<3;++mode){const wchar_t*ids[]={L"LauLanTamBao_1",L"LauLanTamBao_2",L"LauLanTamBao_3"};const wchar_t*names[]={L"Lâu Lan Tầm Bảo (thường)",L"Lâu Lan Tầm Bảo (nhanh)",L"Lâu Lan Tầm Bảo (siêu tốc)"};auto p=Base(ids[mode],names[mode],101,5,384,5200,7700,names[mode]);p.steps.push_back(W(L"Chờ bảo sương",25));Pair(p.steps,L"Tiêu diệt bảo sương",101,2016,1504);p.steps.back().matchAnyVerified=true;Pair(p.steps,L"Trấn Bảo Long Vương",101,2048,2048,800,true);a.push_back(std::move(p));}
 for(int mode=0;mode<3;++mode){const wchar_t*ids[]={L"TucCau_1",L"TucCau_2",L"TucCau_3"};const wchar_t*names[]={L"Túc Cầu (thường)",L"Túc Cầu (nhanh)",L"Túc Cầu (siêu tốc)"};std::wstring dialog=L"Tham gia "+std::wstring(names[mode]);auto p=Base(ids[mode],names[mode],103,4,718,9300,9170,dialog.c_str());p.steps.push_back(W(L"Chờ túc cầu",25));Pair(p.steps,L"Tiêu diệt túc cầu",103,1312,3072);p.steps.back().matchAnyVerified=true;Pair(p.steps,L"Tôn Mỹ Mỹ",103,1952,2272,800,true);a.push_back(std::move(p));}
 {auto p=Base(L"Q1_LauLan",L"Hoàng Kim Chi Liên",108,5,385,9400,8000,L"Hoàng Kim Chi Liên");
  constexpr std::uint32_t S12=(1u<<0)|(1u<<1); constexpr std::uint32_t S3456=(1u<<2)|(1u<<3)|(1u<<4)|(1u<<5);
  p.steps.push_back(M(L"HKCL P1 • 1-2 • A1",108,9394,3009,120,S12,21));
  p.steps.push_back(M(L"HKCL P1 • 1-2 • A2",108,4598,3028,120,S12,21));
  p.steps.push_back(M(L"HKCL P1 • 3-6 • B1",108,4029,1447,120,S3456,21));
  p.steps.push_back(M(L"HKCL P1 • 3-6 • B2",108,4691,1476,120,S3456,21));
  p.steps.push_back(M(L"HKCL P1 • 3-6 • B3",108,5129,2263,120,S3456,21));
  p.steps.push_back(M(L"HKCL • HỘI QUÂN 1",108,4316,2283));
  {auto f=F(L"HKCL • QUÉT SẠCH 1",L"",1000);f.mapID=108;f.x=4316;f.y=2283;f.matchAnyVerified=true;p.steps.push_back(std::move(f));}

  p.steps.push_back(M(L"HKCL P2 • 1-2 • A1",108,6513,6102,120,S12,22));
  p.steps.push_back(M(L"HKCL P2 • 1-2 • A2",108,5603,6612,120,S12,22));
  p.steps.push_back(M(L"HKCL P2 • 3-6 • B1",108,5316,5373,120,S3456,22));
  p.steps.push_back(M(L"HKCL P2 • 3-6 • B2",108,4935,6028,120,S3456,22));
  p.steps.push_back(M(L"HKCL • HỘI QUÂN 2",108,5712,6114));
  {auto f=F(L"HKCL • QUÉT SẠCH 2",L"",1000);f.mapID=108;f.x=5712;f.y=6114;f.matchAnyVerified=true;p.steps.push_back(std::move(f));}

  p.steps.push_back(M(L"HKCL P3 • 1-2 • A1",108,3363,4978,120,S12,23));
  p.steps.push_back(M(L"HKCL P3 • 1-2 • A2",108,1888,4954,120,S12,23));
  p.steps.push_back(M(L"HKCL P3 • 3-6 • B1",108,3499,6425,120,S3456,23));
  p.steps.push_back(M(L"HKCL P3 • 3-6 • B2",108,1755,6531,120,S3456,23));
  const int hx[]={2601,2120,5344,6330}; const int hy[]={5796,6858,2880,6655};
  for(int i=0;i<4;++i){p.steps.push_back(M((L"HKCL • HỘI/ĐÁNH "+std::to_wstring(i+3)).c_str(),108,hx[i],hy[i]));
    auto f=F((L"HKCL • QUÉT SẠCH "+std::to_wstring(i+3)).c_str(),L"",1000);f.mapID=108;f.x=hx[i];f.y=hy[i];f.matchAnyVerified=true;p.steps.push_back(std::move(f));}
  a.push_back(std::move(p));}
 {auto p=Base(L"Q2_LauLan",L"Huyền Phật Châu",109,5,385,9400,8000,L"Huyền Phật Châu");
  constexpr std::uint32_t S12=(1u<<0)|(1u<<1); constexpr std::uint32_t S3456=(1u<<2)|(1u<<3)|(1u<<4)|(1u<<5);
  p.steps.push_back(M(L"HPC P1 • 1-2",109,5784,2927,120,S12,31));
  p.steps.push_back(M(L"HPC P1 • 3-6",109,4377,1923,120,S3456,31));
  p.steps.push_back(M(L"HPC • HỘI/ĐÁNH 1",109,5262,2414));
  {auto f=F(L"HPC • QUÉT SẠCH 1",L"",1000);f.mapID=109;f.x=5262;f.y=2414;f.matchAnyVerified=true;p.steps.push_back(std::move(f));}
  p.steps.push_back(M(L"HPC P2 • 1-2",109,2654,5796,120,S12,32));
  p.steps.push_back(M(L"HPC P2 • 3-6",109,2309,3825,120,S3456,32));
  const int x[]={2332,5788,5881,4310,2322,2601,4040}; const int y[]={4713,5942,2901,1906,3831,5789,4579};
  for(int i=0;i<7;++i){p.steps.push_back(M((L"HPC • HỘI/ĐÁNH "+std::to_wstring(i+2)).c_str(),109,x[i],y[i]));
    auto f=F((L"HPC • QUÉT SẠCH "+std::to_wstring(i+2)).c_str(),L"",1000);f.mapID=109;f.x=x[i];f.y=y[i];f.matchAnyVerified=true;p.steps.push_back(std::move(f));}
  a.push_back(std::move(p));}
 {auto p=Base(L"Q3_LauLan",L"Dung Nham Chi Địa",110,5,386,4000,8500,L"Dung Nham Chi Địa");
  const int x[]={6637,6724,5849,5803,4909,4760,3391,2270,1474,2308,2586,1741,1730,2691,2790,1689,1649,2852,4979,4831,5170,6402,6537,6569,5273,4645,1729};
  const int y[]={1830,2788,2887,1948,1861,2931,1385,1525,1625,2980,4088,4184,5159,5134,6164,6176,6904,6920,5858,5076,4451,4584,5724,6917,6670,6327,6868};
  for(int i=0;i<27;++i){p.steps.push_back(M((L"DUNG NHAM • ĐIỂM "+std::to_wstring(i+1)).c_str(),110,x[i],y[i]));
    auto f=F((L"DUNG NHAM • QUÉT SẠCH "+std::to_wstring(i+1)).c_str(),L"",950);f.mapID=110;f.x=x[i];f.y=y[i];f.matchAnyVerified=true;p.steps.push_back(std::move(f));}
  a.push_back(std::move(p));}
 {auto p=Base(L"AcTacTaoPhan",L"Ác Tặc Tạo Phản • CHỜ ENTRY",0,0,0,0,0,L"");
  const int x[]={3127,3318,2243,1065,1345,708}; const int y[]={1548,3018,3399,3276,1808,1032};
  for(int i=0;i<6;++i){p.steps.push_back(M((L"ÁC TẶC • STEP "+std::to_wstring(i+1)).c_str(),0,x[i],y[i]));
    auto f=F((L"ÁC TẶC • QUÉT SẠCH "+std::to_wstring(i+1)).c_str(),L"",900);f.mapID=0;f.x=x[i];f.y=y[i];f.matchAnyVerified=true;p.steps.push_back(std::move(f));}
  a.push_back(std::move(p));}
 {auto p=Base(L"SatTinh",L"Thập Nhị Sát Tinh",111,2,41,4170,7840,L"Khiêu chiến Thập Nhị Sát Tinh");p.steps.push_back(W(L"Chờ Sát Tinh",25));p.steps.push_back(M(L"Điểm Sát Tinh",111,2037,2550));auto f=F(L"Sát Tinh theo level-band",L"",1000,true);f.matchAnyVerified=true;p.steps.push_back(f);a.push_back(std::move(p));}
 {auto p=Base(L"PMVL",L"Yến Tử Ô",105,25,721,2200,6400,L"Thảo phạt Yến Tử Ô");p.steps.push_back(P(L"Cổng 1",105,5670,1090));Pair(p.steps,L"Đợt 1",105,2660,1520);p.steps.push_back(P(L"Cổng 2",105,2080,1420));Pair(p.steps,L"Đợt 2",105,7300,2400);p.steps.push_back(P(L"Cổng 3",105,5830,5400));Pair(p.steps,L"Đợt 3",105,4800,5500);Pair(p.steps,L"Đợt 4",105,2000,4600);Pair(p.steps,L"Đợt 5",105,1900,6200);for(auto& s:p.steps) if(s.kind==StepKind::Fight&&s.monsterName.find(L"Đợt")==0) s.matchAnyVerified=true;a.push_back(std::move(p));}
 {auto p=Base(L"PhiKH",L"Diệt Phỉ Kính Hồ",107,30,301,6400,8430,L"Diệt phỉ Kính Hồ");p.steps.push_back(W(L"Chờ thủy phỉ",25));Pair(p.steps,L"Kính Hồ Thuỷ Phỉ",107,992,864);Pair(p.steps,L"Kính Hồ Thủy Phỉ Đầu Lĩnh",107,4384,4832,800,true);Pair(p.steps,L"Kính Hồ Thủy Phỉ Đầu Lĩnh",107,4448,4800,800,true);a.push_back(std::move(p));}
 return a;
}
}
