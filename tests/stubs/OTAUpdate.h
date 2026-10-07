#pragma once
struct OTAUpdate {
 static constexpr int OTA_ERROR_NONE=0;
 inline static int beginResult=0,startResult=1000,progress=0,verifyResult=0,updateResult=-26;
 inline static int beginCalls=0,startCalls=0,verifyCalls=0,updateCalls=0,resetCalls=0;
 int begin(const char*){++beginCalls;return beginResult;}
 int startDownload(const char*,const char*){++startCalls;return startResult;}
 int downloadProgress(){return progress;}
 int verify(){++verifyCalls;return verifyResult;}
 int update(const char*){++updateCalls;return updateResult;}
 int reset(){++resetCalls;return 0;}
};
