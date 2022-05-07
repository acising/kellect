#include <wbemidl.h>
#include <wmistr.h>
#include <evntrace.h>
#include <vector>
#include <time.h>
#include <tdh.h> //PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD
#include <in6addr.h>
#include <conio.h>
#include <strsafe.h>
#include <fstream>
#include <set>
#include <iostream>
#include <stdio.h>
#include <string>

#include "process/etw_configuration.h"
#include "process/customer_parse.h"
#include "process/event_parse.h"
#include "process/multithread_configuration.h"

//ETWConfiguration::ETWConfiguration(ULONG64 enabledFalg)
ETWConfiguration::ETWConfiguration(ULONG64 enabledFalg)
{
   //Preemption();
                    
    //enable_flag = 0
    //    | EVENT_TRACE_FLAG_PROCESS			// process start & end              *
    //    | EVENT_TRACE_FLAG_THREAD			// thread start & end               *
    //    | EVENT_TRACE_FLAG_IMAGE_LOAD 		// image load                       *
    //    | EVENT_TRACE_FLAG_FILE_IO          // file IO                          *
    //    |EVENT_TRACE_FLAG_FILE_IO_INIT
    //    | EVENT_TRACE_FLAG_DISK_FILE_IO     // requires disk IO                 *
    //    //| EVENT_TRACE_FLAG_REGISTRY         // registry calls                   *
    //    | EVENT_TRACE_FLAG_CSWITCH          // context switches                 *
    //    | EVENT_TRACE_FLAG_SYSTEMCALL       // system calls                     *
    //    //| EVENT_TRACE_FLAG_ALPC             // ALPC traces                      
    //    //| EVENT_TRACE_FLAG_DISK_IO          // physical disk IO                 * 
    //    //| EVENT_TRACE_FLAG_DISK_IO_INIT     // physical disk IO initiation
    //    | EVENT_TRACE_FLAG_NETWORK_TCPIP     // tcpip send & receive            * 
    //    ;

    enable_flag = enabledFalg;
    logfile_path = L"C:\\logfile.bin";
}

ETWConfiguration& ETWConfiguration::operator=(const ETWConfiguration& config) {

    if (this == &config)
    {
        return *this;
    }
    enable_flag = config.enable_flag;
    logfile_path = config.logfile_path;

    return *this;
}


PEVENT_TRACE_PROPERTIES
ETWConfiguration::AllocateTraceProperties(
    _In_opt_ PWSTR LoggerName,
    _In_opt_ PWSTR LogFileName,
    _In_opt_ BOOLEAN isSysLogger,
    _In_opt_ BOOLEAN isRealTimeSession){

    PEVENT_TRACE_PROPERTIES TraceProperties = nullptr;
    ULONG BufferSize;

    // Allocate memory for the session properties. The memory must
    // be large enough to include the log file name and session name,
    // which get appended to the end of the session properties structure.
    BufferSize = sizeof(EVENT_TRACE_PROPERTIES) +
        (MAXIMUM_SESSION_NAME + MAX_PATH) * sizeof(WCHAR);

    TraceProperties = (PEVENT_TRACE_PROPERTIES)malloc(BufferSize);
    if (TraceProperties == nullptr) {
        wprintf(L"Unable to allocate %d bytes for properties structure.\n", BufferSize);
        goto Exit;
    }

    //
    // Set the session properties.
    //

    ZeroMemory(TraceProperties, BufferSize);
    TraceProperties->Wnode.BufferSize = BufferSize;
    TraceProperties->Wnode.ClientContext = 2; // //QPC clock resolution=1; systemtime=2 low accuracy,but can been translate to standard time 
    TraceProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;  //indicate that the structure contains event tracing information.

    /*
    EnableFlags is only valid for system loggers;the identifier of the system loggers are as follow
    trace sessions that are started using the EVENT_TRACE_SYSTEM_LOGGER_MODE logger mode flag,
    the KERNEL_LOGGER_NAME session name, the SystemTraceControlGuid session GUID, or the GlobalLoggerGuid session GUID.
    */
    if (isSysLogger) {
        TraceProperties->Wnode.Guid = SystemTraceControlGuid;
        TraceProperties->EnableFlags = this->enable_flag;
    }

    TraceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
    TraceProperties->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES) +
        (MAXIMUM_SESSION_NAME * sizeof(WCHAR));
    // Set the session properties. You only append the log file name
    // to the properties structure; the StartTrace function appends
    // the session name for you.
    if (isRealTimeSession) {
        TraceProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE | EVENT_TRACE_SYSTEM_LOGGER_MODE;
    }
    else {
        TraceProperties->LogFileMode = EVENT_TRACE_FILE_MODE_SEQUENTIAL | EVENT_TRACE_SYSTEM_LOGGER_MODE;
        //StringCbCopy((LPWSTR)((char*)TraceProperties + TraceProperties->LogFileNameOffset), (logfile_path.length() + 1) * 2, logfile_path.c_str());
    }

    TraceProperties->MaximumFileSize = 100; // Limit file size to 100MB max
    TraceProperties->BufferSize = 512; // Use 512KB trace buffer
    TraceProperties->MinimumBuffers = 1024;
    TraceProperties->MaximumBuffers = 1024;
    //TraceProperties->EnableFlags = EVENT_TRACE_FLAG_DISK_IO;

    if (LoggerName != nullptr) {
        StringCchCopyW((LPWSTR)((PCHAR)TraceProperties + TraceProperties->LoggerNameOffset),
            MAXIMUM_SESSION_NAME,
            LoggerName);
    }

    if (LogFileName != nullptr) {
        StringCchCopyW((LPWSTR)((PCHAR)TraceProperties + TraceProperties->LogFileNameOffset),
            MAX_PATH,
            LogFileName);
    }

Exit:
    return TraceProperties;
}

int ETWConfiguration::MainSessionConfig(bool real_time_switch) {
start:
    ULONG status = ERROR_SUCCESS;
    TRACEHANDLE SessionHandle = 0;
    EVENT_TRACE_PROPERTIES* mainSessionProperties = nullptr;
    ULONG BufferSize = 0;
    //PWSTR LoggerName = (PWSTR)L"MyTrace";

    mainSessionProperties = AllocateTraceProperties(NULL, NULL,true);

    // Create the trace session.
    status = StartTrace(&SessionHandle, KERNEL_LOGGER_NAME, mainSessionProperties);

    if (ERROR_SUCCESS != status)
    {
        if (ERROR_ALREADY_EXISTS == status)
        {
            status = ControlTrace(SessionHandle, KERNEL_LOGGER_NAME, mainSessionProperties, EVENT_TRACE_CONTROL_STOP);
            //wprintf(L"The NT Kernel Logger session is already in use.\n");
            //wprintf(L"The NT Kernel Logger session is already in use and will be finished.\n");
            //wprintf(L"restart the NT Kernel Logger automaticly... .\n");
            goto start;
        }
        else
        {
            wprintf(L"EnableTrace() failed with %lu\n", status);
            goto cleanup;
        }

        //goto cleanup;
    }

    //wprintf(L"Press any key to end trace session\n\n ");
    wprintf(L"Press any key to end trace session..\n\n ");
    // _getch();

    if (real_time_switch) {
        EventCallstack::initCallStackTracing(SessionHandle);

        //SetupEventConsumer((LPWSTR)KERNEL_LOGGER_NAME,TRUE);
        std::thread tt(&ETWConfiguration::SetupEventConsumer, this, (LPWSTR)KERNEL_LOGGER_NAME, TRUE);
        tt.detach();
        //tt.join();

        while (1) {

            std::this_thread::sleep_for(std::chrono::microseconds(1000000));
            status = ControlTrace(SessionHandle, KERNEL_LOGGER_NAME, mainSessionProperties, EVENT_TRACE_CONTROL_QUERY);
            if (ERROR_SUCCESS == status)
            {
                std::this_thread::sleep_for(std::chrono::microseconds(20000000));
                std::cout <<
                    "  BuffersWritten:" << mainSessionProperties->BuffersWritten <<
                    "  FreeBuffers:" << mainSessionProperties->FreeBuffers <<
                    "  NumberOfBuffers:" << mainSessionProperties->NumberOfBuffers <<
                    "  EventsLost:" << mainSessionProperties->EventsLost
                    << std::endl;
                
                if (mainSessionProperties->EventsLost > 0)
                    int a = 0; 
            }
        }

        goto cleanup; 
    } 
    else {
        wprintf(L"\n");
        getchar();
    }

    return 0;

cleanup:

    if (SessionHandle)
    {
        status = ControlTrace(SessionHandle, KERNEL_LOGGER_NAME, mainSessionProperties, EVENT_TRACE_CONTROL_STOP);

        if (ERROR_SUCCESS != status)
        {
            wprintf(L"ControlTrace(stop) failed with %lu\n", status);
        }
    }

    if (mainSessionProperties)
        free(mainSessionProperties);

    return 0;

}

int ETWConfiguration::SubSessionConfig4XMLProvider(bool real_time_switch,GUID providerGUID,ULONG matchAnyKeywords, PWSTR privateLoggerName) {

start:
    ULONG status = ERROR_SUCCESS;
    TRACEHANDLE SessionHandle = 0;
    EVENT_TRACE_PROPERTIES* subSessionProperties = nullptr;
    ULONG BufferSize = 0;
    //PWSTR LoggerName = (PWSTR)L"subSession";
    subSessionProperties = AllocateTraceProperties(privateLoggerName, NULL, FALSE);

    // Create the trace session.
    status = StartTraceW((PTRACEHANDLE)&SessionHandle, privateLoggerName, subSessionProperties);


    if (ERROR_SUCCESS != status)
    {
        wprintf(L"GetProcAddress failed with %lu.\n", status = GetLastError());

        if (ERROR_ALREADY_EXISTS == status)
        {


            status = ControlTraceA(SessionHandle, (LPCSTR)privateLoggerName, subSessionProperties, EVENT_TRACE_CONTROL_STOP);
            //wprintf(L"The NT Kernel Logger session is already in use.\n");
            wprintf(L"The NT Kernel Logger session is already in use and will be finished.\n");
            wprintf(L"restart the NT Kernel Logger automaticly... .\n");

            goto start;
        }
        else
        {
            wprintf(L"EnableTrace() failed with %lu\n", status);
            goto cleanup;
        }
    }

    status = EnableTraceEx2(SessionHandle, &SystemTraceControlGuid, EVENT_CONTROL_CODE_ENABLE_PROVIDER, TRACE_LEVEL_INFORMATION, 0, 0, 0, nullptr);
    //status = EnableTraceEx2(SessionHandle, &providerGUID, EVENT_CONTROL_CODE_ENABLE_PROVIDER, TRACE_LEVEL_INFORMATION, matchAnyKeywords, 0, 0, nullptr);
    status = EnableTraceEx2(SessionHandle, &providerGUID, EVENT_CONTROL_CODE_CAPTURE_STATE, TRACE_LEVEL_INFORMATION, matchAnyKeywords, 0, 0, nullptr);

    //wprintf(L"Press any key to end trace session ");
    // _getch();


    if (real_time_switch) {
        SetupEventConsumer(privateLoggerName,FALSE);
        goto cleanup;
    }
    else {
        wprintf(L"�������ļ���ʽ�����־��Ϣ�����������ֹ��\n");
        getchar();
    }

cleanup:

    if (SessionHandle)
    {
        status = ControlTrace(SessionHandle, NULL, subSessionProperties, EVENT_TRACE_CONTROL_STOP);
        status = EnableTraceEx2(SessionHandle, &providerGUID, EVENT_CONTROL_CODE_DISABLE_PROVIDER, TRACE_LEVEL_INFORMATION, 0, 0, 0, nullptr);

        if (ERROR_SUCCESS != status)
        {
            wprintf(L"ControlTrace(stop) failed with %lu\n", status);
            wprintf(L"cleanup SubSession Config failed with %lu.\n", status = GetLastError());

        }
    }

    if (subSessionProperties)
        free(subSessionProperties);

    return 0;
}

INT ETWConfiguration::SubSessionConfig4MOFProvider(
    bool real_time_switch,
    ULONG enabledFlags,
    PWSTR privateLoggerName) {

start:
    ULONG status = ERROR_SUCCESS;
    TRACEHANDLE SessionHandle = 0;
    EVENT_TRACE_PROPERTIES* subSessionProperties = nullptr;
    ULONG BufferSize = 0;
    //PWSTR LoggerName = (PWSTR)L"subSession";
    subSessionProperties = AllocateTraceProperties(privateLoggerName, NULL, FALSE);

    subSessionProperties->EnableFlags = enabledFlags;
    // Create the trace session.
    status = StartTraceW((PTRACEHANDLE)&SessionHandle, privateLoggerName, subSessionProperties);


    if (ERROR_SUCCESS != status)
    {
        wprintf(L"GetProcAddress failed with %lu.\n", status = GetLastError());

        if (ERROR_ALREADY_EXISTS == status)
        {
            //Ĭ��subsession���Զ���session
            status = ControlTrace(SessionHandle, (LPCSTR)privateLoggerName, subSessionProperties, EVENT_TRACE_CONTROL_STOP);
            //wprintf(L"The NT Kernel Logger session is already in use.\n");
            wprintf(L"The NT Kernel Logger session is already in use and will be finished.\n");
            wprintf(L"restart the NT Kernel Logger automaticly... .\n");

            goto start;
        }
        else
        {
            wprintf(L"EnableTrace() failed with %lu\n", status);
            goto cleanup;
        }
    }

    //Status = EnableTraceEx2(SessionHandle, &SystemTraceControlGuid, EVENT_CONTROL_CODE_ENABLE_PROVIDER, TRACE_LEVEL_INFORMATION, 0x10, 0, 0, nullptr);
    //status = EnableTraceEx2(SessionHandle, &providerGUID, EVENT_CONTROL_CODE_ENABLE_PROVIDER, TRACE_LEVEL_INFORMATION, matchAnyKeywords, 0, 0, nullptr);

    //wprintf(L"Press any key to end trace session ");
    // _getch();


    if (real_time_switch) {
        SetupEventConsumer(privateLoggerName, FALSE);
        goto cleanup;
    }
    else {
        wprintf(L"output to a file,Press any key to end trace session..\n");
        getchar();
    }

cleanup:

    if (SessionHandle)
    {
        status = ControlTrace(SessionHandle, NULL, subSessionProperties, EVENT_TRACE_CONTROL_STOP);
        //status = EnableTraceEx2(SessionHandle, &providerGUID, EVENT_CONTROL_CODE_DISABLE_PROVIDER, TRACE_LEVEL_INFORMATION, 0, 0, 0, nullptr);

        if (ERROR_SUCCESS != status)
        {
            wprintf(L"ControlTrace(stop) failed with %lu\n", status);
            wprintf(L"cleanup SubSession Config failed with %lu.\n", status = GetLastError());

        }
    }

    if (subSessionProperties)
        free(subSessionProperties);

    return 0;
}


INT ETWConfiguration::AllocateTraceLogFile(
    _In_opt_ PWSTR LoggerName,
    EVENT_TRACE_LOGFILE& event_logfile,
    BOOLEAN mainConsumer,
    _In_opt_ BOOLEAN isRealTimeSession) {
    
    //event_logfile = (PEVENT_TRACE_LOGFILE)malloc(sizeof(EVENT_TRACE_LOGFILE));
    ZeroMemory(&event_logfile, sizeof(EVENT_TRACE_LOGFILE));

    event_logfile.LoggerName = reinterpret_cast<LPSTR>((LPWSTR) LoggerName);
    event_logfile.ProcessTraceMode = PROCESS_TRACE_MODE_EVENT_RECORD;
    // consum_event() is the callback function. should be writen in this class.
    EventParser EventParser;
    if(isRealTimeSession)
        event_logfile.ProcessTraceMode |= PROCESS_TRACE_MODE_REAL_TIME;

    if(mainConsumer)
        event_logfile.EventRecordCallback = (PEVENT_RECORD_CALLBACK)(EventParser.ConsumeEventMain);
    else
        event_logfile.EventRecordCallback = (PEVENT_RECORD_CALLBACK)(EventParser.ConsumeEventSub);

    return 0;
}

void ETWConfiguration::SetupEventConsumer(LPWSTR loggerName,BOOLEAN isMainSession) {

    EVENT_TRACE_LOGFILE event_logfile;
    TRACEHANDLE event_logfile_handle;
    BOOL event_usermode = FALSE;
    DOUBLE timeStampScale;
    TRACE_LOGFILE_HEADER* event_logfile_header;
    ULONG status = ERROR_SUCCESS;
    TDHSTATUS temp_status;

    event_logfile_header = &(event_logfile.LogfileHeader);
    status=AllocateTraceLogFile(loggerName, event_logfile,isMainSession);

    if (status != ERROR_SUCCESS) {
        wprintf(L"AllocateTraceLogFile failed with %lu\n", status);
        goto cleanup;
    }
   // event_logfile_handle = OpenTrace(event_logfile);
    event_logfile_handle = OpenTrace(&event_logfile);

    if (INVALID_PROCESSTRACE_HANDLE == event_logfile_handle) {
        wprintf(L"OpenTrace failed with %lu\n", GetLastError());
        goto cleanup;
    }
    
    event_usermode = event_logfile_header->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE;

    if (event_logfile_header->PointerSize != sizeof(PVOID)) {
        event_logfile_header = (PTRACE_LOGFILE_HEADER)((PUCHAR)event_logfile_header +
            2 * (event_logfile_header->PointerSize - sizeof(PVOID)));
    }

    // If everything go well, the program will be block here.
    // to perform the callback function defined in EventRecordCallback property
    temp_status = ProcessTrace(&event_logfile_handle, 1, 0, 0);  


    if (temp_status != ERROR_SUCCESS && temp_status != ERROR_CANCELLED) {
        wprintf(L"ProcessTrace failed with %lu\n", temp_status);
        goto cleanup;
    }

cleanup:
    if (INVALID_PROCESSTRACE_HANDLE != event_logfile_handle) {
        temp_status = CloseTrace(event_logfile_handle);
    }
}


int ETWConfiguration::ETWSessionConfig(bool real_time_switch)
{

    //bool succ1 = SubSessionConfig4MOFProvider(real_time_switch, EVENT_TRACE_FLAG_PROCESS,(LPWSTR)L"MyTrace1");
    
    //ETWConfiguration etwConfiguration;
    //this->callback = callback;

    //std::thread th2(&ETWConfiguration::MainSessionConfig, real_time_switch);
    //std::thread th2(&ETWConfiguration::my_print);

    //std::thread t1 = etwConfiguration.execThread();
    //std::thread th1(&ETWConfiguration::SubSessionConfig4XMLProvider, &etwConfiguration, real_time_switch, Kernel_Process, 0x10, (LPWSTR)L"MyTrace1");
    //std::thread th2 = etwConfiguration.startThread4MainSessionConfig(real_time_switch);
    
    //th1.join();
    //th2.join();
  
    //MainSessionConfig.
    MainSessionConfigThread  t1(*this,real_time_switch);
    t1.startThread();
    t1.wait();
    //XMLSubSessionConfigThread t1(real_time_switch, (LPWSTR)L"MyTrace1", Kernel_Process, 0x10);
    //t1.startThread();
    //t1.wait();

    //bool succ2 = MainSessionConfig(real_time_switch);

    //bool succ1 = SubSessionConfig4XMLProvider(real_time_switch, Kernel_Process, 0x10, (LPWSTR)L"MyTrace1");


    std::cout << "I am the main Thread" << std::endl;
    return 1;
}
