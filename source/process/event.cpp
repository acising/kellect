#include "tools/logger.h"
#include "tools/tools.h"
#include "process/sub_event.h"
#include <fstream>
#include <iostream> 
#include <regex>

std::map<EventIdentifier*, std::vector<std::string>,EventIdentifierSortCriterion > BaseEvent::eventPropertiesMap;		//映射了event中不同的参数
std::set <std::string> BaseEvent::propertyNameSet;
std::vector <std::string> BaseEvent::propertyNameVector;
std::map <std::string, int> BaseEvent::propertyName2IndexMap;
std::set <EventIdentifier*, EventIdentifierSortCriterion> BaseEvent::eventIdentifierSet;

//ReadWriteMap<ULONG64, std::string> EventFile::fileKey2Name;
std::map<ULONG64, std::string> EventFile::fileKey2Name;
//ReadWriteMap<ULONG64, std::string> EventFile::fileObject2Name;
std::map<ULONG64, std::string> EventFile::fileObject2Name;

std::map<int, std::string> EventProcess::processID2Name;
std::map<std::string, int> EventProcess::processName2ID;
//ReadWriteMap<int, std::string> EventProcess::processID2Name;
//ReadWriteMap<std::string, int> EventProcess::processName2ID;
//std::set<INT64> EventProcess::processIDSet;

ReadWriteMap <int, std::set<Module*, ModuleSortCriterion> > EventImage::processID2Modules;
//std::map <int, std::set<Module*, ModuleSortCriterion> > EventImage::processID2Modules;
std::map < std::string, std::set<MyAPI*, MyAPISortCriterion> > EventImage::usedModulesName2APIs;

extern std::map<ULONG64, std::string> addr2FuncName;
extern std::map<ULONG64, std::string> addr2FuncNameUsed;
//INT64 Tools::String2INT64(std::string s);  //将string转换为INT64


dataType* BaseEvent::getProperty(int propertyNameIndex) {

	if (BaseEvent::propertyNameVector.size() <= propertyNameIndex ) {
		MyLogger::writeLog("propertyNameIdex 超出了EventIdentifier::propertyNameVector的大小！\n");
		exit(-1);
	}
	if (propertyNameIndex < 0) {
		MyLogger::writeLog("getProperty propertyNameIdex 为负数\n");
		exit(-1);
	}

	std::string propertyName = BaseEvent::propertyNameVector[propertyNameIndex];
	//event doen't have this property
	if (!properties.count(propertyName)) {

		propertyName = "unknownProperty";
		properties.insert(std::map <std::string, dataType*>::value_type(propertyName, nullptr));
		MyLogger::writeLog("未找到事件对应的propertyName\n");
	}

	return this->properties[propertyName];
}

void BaseEvent::setProperty(int propertyNameIdex, dataType* dt) {

	if (BaseEvent::propertyNameVector.size() <= propertyNameIdex) {
		MyLogger::writeLog("propertyNameIdex 超出了EventIdentifier::propertyNameVector的大小！\n");
		exit(-1);
	}

	if (propertyNameIdex < 0) {
		MyLogger::writeLog("setProperty propertyNameIdex 为负数\n");
		exit(-1);
	}
	std::string propertyName = BaseEvent::propertyNameVector[propertyNameIdex];

	properties[propertyName] = dt;
}

