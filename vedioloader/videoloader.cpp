// vedioloader.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include <Windows.h>
#include "HCNetSDK.h"
#include <string>
#include <time.h>

#define CCT (+8)
const int splitMinuteTime = 10;
struct ConnectInfo{
	char ip[16];
	int port;
	int chanel;
	char userName[256];
	char password[256];
	NET_DVR_TIME startTime;
	NET_DVR_TIME endTime;
	char* path=NULL;
};
int getVedioByTime(int UserID, LONG chanel, NET_DVR_TIME *startTime, NET_DVR_TIME *endTime,char* path){
	char storeFileName[256];
	if (path == NULL){
		sprintf_s(storeFileName, "./video/%d.%d.%d.%d.%d.%d-%d.%d.%d.%d.%d.%d.mp4", startTime->dwYear, startTime->dwMonth, startTime->dwDay, startTime->dwHour, startTime->dwMinute, startTime->dwSecond,
			endTime->dwYear, endTime->dwMonth, endTime->dwDay, endTime->dwHour, endTime->dwMinute, endTime->dwSecond);
		path = storeFileName;
	}
	std::string s_path(path);
	std::size_t l_position=s_path.find_last_of("/");
	std::string dir = s_path.substr(0, l_position);
	
	std::cout <<"path:"<< path << std::endl;
	NET_DVR_PLAYCOND struFileCond = { 0 };
	//通道号
	struFileCond.dwChannel = chanel;
	//开始时间
	struFileCond.struStartTime = *startTime;
	//结束时间
	struFileCond.struStopTime = *endTime;
	int IFindHandle = NET_DVR_GetFileByTime_V40(UserID, path, &struFileCond);
	if (IFindHandle < 0){
		std::cout << "error:find file fail " << NET_DVR_GetLastError() << std::endl;
		return -1;
	}
	if (!NET_DVR_PlayBackControl_V40(IFindHandle, NET_DVR_PLAYSTART, NULL, 0, NULL, NULL)){
		std::cout << "error:play back control failed " << NET_DVR_GetLastError() << std::endl;
		NET_DVR_Logout(UserID);
		NET_DVR_Cleanup();
		return -1;
	}
	//视频下载
	int nPos = 0;
	for (nPos = 0; nPos < 100 && nPos >= 0; nPos = NET_DVR_GetDownloadPos(IFindHandle)){
		std::cout <<"progress:"<< nPos << std::endl;
		Sleep(5000);
	}
	if (nPos<0 || nPos>100){
		std::cout << "error:downloading problem " << NET_DVR_GetLastError() << std::endl;
		NET_DVR_Logout(UserID);
		NET_DVR_Cleanup();
		return -1;
	}
	if (!NET_DVR_StopPlayBack(IFindHandle)){
		std::cout << "error:stop failed " << NET_DVR_GetLastError() << std::endl;
		return -1;
	}
	std::cout << "progress:" << nPos << std::endl;
	return 0;
}
/*
把时间格式转化成NET_DVR_TIME
*/
void parseTime(char *t, NET_DVR_TIME &ndt){
	std::string dateTime = t;
	std::size_t y, mon, d, h, m;
	y = dateTime.find_first_of("-");
	ndt.dwYear = atoi(dateTime.substr(0, y).c_str());
	std::string tempStr = dateTime.substr(y + 1);
	mon = tempStr.find_first_of("-");
	ndt.dwMonth = atoi(tempStr.substr(0, mon).c_str());
	tempStr = tempStr.substr(mon + 1);
	d = tempStr.find_first_of(" ");
	ndt.dwDay = atoi(tempStr.substr(0, d).c_str());
	tempStr = tempStr.substr(d + 1);
	h = tempStr.find_first_of(":");
	ndt.dwHour = atoi(tempStr.substr(0, h).c_str());
	tempStr = tempStr.substr(h + 1);
	m = tempStr.find_first_of(":");
	ndt.dwMinute = atoi(tempStr.substr(0, m).c_str());
	ndt.dwSecond = atoi(tempStr.substr(m + 1).c_str());
}

/*
把输入的参数转化成对应的值。
*/
ConnectInfo parseParam(int argc, char *arg[]){
	ConnectInfo ci;
	for (int i = 1; i < argc; i += 2){
		std::string param(arg[i]);
		char* value = arg[i + 1];

		if (param == "-ip"){
			sprintf_s(ci.ip, "%s", value);
		}else if (param == "-port"){
			ci.port = atoi(value);
		}else if (param == "-c"){
			ci.chanel = atoi(value);
		}else if (param == "-u"){
			sprintf_s(ci.userName, "%s", value);
		}else if (param == "-p"){
			sprintf_s(ci.password, "%s", value);
		}else if (param == "-st"){
			parseTime(value, ci.startTime);
		}else if (param == "-et"){
			parseTime(value, ci.endTime);
		}
		else if (param == "-path"){
			ci.path = value;
		}
	}
	return ci;
}
////计算年份是否为闰年、
//bool isLeapYear(int year){
//	if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)
//		return true;
//	else
//		return false;
//}
NET_DVR_TIME getTimeSum(NET_DVR_TIME *beginTime, double seconds){
	NET_DVR_TIME timeSum;
	struct tm btime,*etime;
	btime.tm_year = beginTime->dwYear - 1900;
	btime.tm_mon = beginTime->dwMonth - 1;
	btime.tm_mday = beginTime->dwDay;
	btime.tm_hour = beginTime->dwHour;
	btime.tm_min = beginTime->dwMinute;
	btime.tm_sec = beginTime->dwSecond;
	time_t bSec=mktime(&btime);
	double totalSec = bSec + seconds;
	time_t eSec = (time_t)totalSec;
	char* resultTime = ctime(&eSec);
	etime = localtime(&eSec);
	timeSum.dwYear = etime->tm_year+1900;
	timeSum.dwMonth = etime->tm_mon + 1;
	timeSum.dwDay = etime->tm_mday;
	timeSum.dwHour = etime->tm_hour;
	timeSum.dwMinute = etime->tm_min;
	timeSum.dwSecond = etime->tm_sec;
	return timeSum;
}
int main(int argc, char* argv[])
{
	ConnectInfo ci;
	struct tm stime, etime;
	ci = parseParam(argc, argv);
	//起始时间
	stime.tm_year = (int)(ci.startTime.dwYear)-1900;
	stime.tm_mon = (int)ci.startTime.dwMonth-1;//tm_mon值为0-11
	stime.tm_mday = (int)ci.startTime.dwDay;
	stime.tm_hour = (int)ci.startTime.dwHour;
	stime.tm_min = (int)ci.startTime.dwMinute;
	stime.tm_sec = (int)ci.startTime.dwSecond;
	//结束时间
	etime.tm_year = (int)ci.endTime.dwYear-1900;
	etime.tm_mon = (int)ci.endTime.dwMonth - 1;
	etime.tm_mday = (int)ci.endTime.dwDay;
	etime.tm_hour = (int)ci.endTime.dwHour;
	etime.tm_min = (int)ci.endTime.dwMinute;
	etime.tm_sec = (int)ci.endTime.dwSecond;
	//计算时间差
	double diffsec = difftime(mktime(&etime), mktime(&stime));
	NET_DVR_Init();
	NET_DVR_SetConnectTime(2000, 1);
	NET_DVR_SetReconnect(10000, true);
	LONG IUserID;
	NET_DVR_DEVICEINFO_V30 struDeviceInfo;
	IUserID = NET_DVR_Login_V30(ci.ip, ci.port, ci.userName, ci.password, &struDeviceInfo);
	if (IUserID < 0){
		std::cout << "error:Login failed " << NET_DVR_GetLastError() << std::endl;
		NET_DVR_Cleanup();
		return -1;
	}
	int startChanel = (int)struDeviceInfo.byStartDChan;
	//判断输入的通道号是否在范围内
	int chanelNum = (int)struDeviceInfo.byIPChanNum;
	if (ci.chanel>chanelNum){
		std::cout << "error:chanel out of arrange" << std::endl;
		return -1;
	}
	int inputChanel = startChanel + ci.chanel - 1;
	int splitNum = diffsec /(splitMinuteTime*60);
	int remTime = (int)diffsec %(splitMinuteTime*60);

	NET_DVR_TIME splitEndTime,splitStartTime;
	splitStartTime = ci.startTime;
	
	for (int i = 0; i < splitNum; i++){
		
		splitEndTime = getTimeSum(&splitStartTime, splitMinuteTime * 60);
		getVedioByTime(IUserID, inputChanel, &splitStartTime, &splitEndTime, ci.path);
		splitStartTime.dwYear=splitEndTime.dwYear;
		splitStartTime.dwMonth = splitEndTime.dwMonth;
		splitStartTime.dwDay = splitEndTime.dwDay;
		splitStartTime.dwHour = splitEndTime.dwHour;
		splitStartTime.dwMinute = splitEndTime.dwMinute;
		splitStartTime.dwSecond = splitEndTime.dwSecond;
	}
	if (remTime>1){
		getVedioByTime(IUserID, inputChanel, &splitStartTime, &ci.endTime, ci.path);
	}
	NET_DVR_Logout(IUserID);
	NET_DVR_Cleanup();
	return 0;
}


