/*
* please enable the data communication output in PMDG!
*/
#include <iostream>
#include <fstream>
#include <windows.h>
#include "string.h"
#include "SimConnect.h"
#include "KnownFolders.h"
#include "PMDG_NG3_SDK.h"
#include <tuple>
#define CE_SERIAL_IMPLEMENTATION
#include "ceSerial.h"


int quit = 0;
int control_state, stick_state, new_stick_state;
int new_button_press, previous_button_press=0;
string port_info;
HANDLE hsimconnect = NULL;

enum Event_ID
{
    EVENT_TAXILIGHT,
    EVENT_WINDOWHEAT1, 
    EVENT_WINDOWHEAT2,
    EVENT_WINDOWHEAT3,
    EVENT_WINDOWHEAT4
};
enum Request_ID
{
    DATA_REQUEST,
};
enum Button_State
{
    UNPRESSED,PRESSED,ERRORCODE
};

int targets[] = {EVENT_TAXILIGHT, EVENT_WINDOWHEAT1, EVENT_WINDOWHEAT2,
                        EVENT_WINDOWHEAT3, EVENT_WINDOWHEAT4};   //所有要控制的目标控件
int present_target = 0; //代表当前控制哪个控件

//give control state a value
void CALLBACK MyDispatchProc1(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext)
{
    switch (pData->dwID)
    {
    case SIMCONNECT_RECV_ID_CLIENT_DATA:
    {
        SIMCONNECT_RECV_CLIENT_DATA* pObjData = (SIMCONNECT_RECV_CLIENT_DATA*)pData;

        switch (pObjData->dwRequestID)
        {
        case DATA_REQUEST:
            PMDG_NG3_Data* pS = (PMDG_NG3_Data*)&pObjData->dwData;
            switch (targets[present_target])
            {
            case EVENT_TAXILIGHT:
                control_state = pS->LTS_TaxiSw;
                break;
            case EVENT_WINDOWHEAT1:
                control_state = pS->ICE_WindowHeatSw[0];
                break;
            case EVENT_WINDOWHEAT2:
                control_state = pS->ICE_WindowHeatSw[1];
                break;
            case EVENT_WINDOWHEAT3:
                control_state = pS->ICE_WindowHeatSw[2];
                break;
            case EVENT_WINDOWHEAT4:
                control_state = pS->ICE_WindowHeatSw[3];
                break;
            }
            
            break;
        }
        break;
    }


    case SIMCONNECT_RECV_ID_QUIT:
    {
        quit = 1;
        break;
    }

    default:
        break;
    }
}

//initialize simconnect and doing matchings
void SimConnect_Initialization()
{
    //keep connecting to the server & opening serial port
    while (1)
    {
        if (SUCCEEDED(SimConnect_Open(&hsimconnect, "Client Event", NULL, 0, NULL, 0)))
        {
            std::cout << "SimConnect OK" << std::endl;
            break;
        }
    }
    //match
    SimConnect_MapClientDataNameToID(hsimconnect, PMDG_NG3_DATA_NAME, PMDG_NG3_DATA_ID);
    SimConnect_MapClientEventToSimEvent(hsimconnect, EVENT_TAXILIGHT, "#69749");
    SimConnect_MapClientEventToSimEvent(hsimconnect, EVENT_WINDOWHEAT1, "#69767");
    SimConnect_MapClientEventToSimEvent(hsimconnect, EVENT_WINDOWHEAT2, "#69768");
    SimConnect_MapClientEventToSimEvent(hsimconnect, EVENT_WINDOWHEAT3, "#69770");
    SimConnect_MapClientEventToSimEvent(hsimconnect, EVENT_WINDOWHEAT4, "#69771");
    SimConnect_AddToClientDataDefinition(hsimconnect, PMDG_NG3_DATA_DEFINITION, 0, sizeof(PMDG_NG3_Data), 0, 0);

    SimConnect_RequestClientData(hsimconnect,
        PMDG_NG3_DATA_ID, DATA_REQUEST, PMDG_NG3_DATA_DEFINITION,
        SIMCONNECT_CLIENT_DATA_PERIOD_SECOND, 0, 0, 0, 0);
}

//get info from serial and transfer it to int(both stick state and button state)
std::tuple<int, int> get_serial_port(ceSerial &com)
{
    int result_stick, result_button;
    bool successflag;
    int state = int(com.ReadChar(successflag))-1;
    
    result_stick = state / 10;
    result_button = state % 10;

    return std::make_tuple(result_stick, result_button);

}

int main()
{
    SimConnect_Initialization(); 
    
    //initialize com
    ceSerial com("\\\\.\\COM5", 9600, 8, 'N', 1);
    std::cout << "Opening port" << std::endl;
    while (com.Open()!=0)
    {
        continue;
    }
    std::cout << "Serial port open!" << std::endl;
    std::cout << "Now you can control the sim!" << std::endl;

  
    while (quit == 0)
    {
        //process controller state
        std::tuple<int, int> result = get_serial_port(com);
        new_stick_state = std::get<0>(result);
        new_button_press = std::get<1>(result);
        present_target = (present_target + ((previous_button_press==0) ? new_button_press : 0)) % 5;
        std::cout << present_target << std::endl;

        SimConnect_CallDispatch(hsimconnect, MyDispatchProc1, NULL);

        //detect stick state change;if changed, send event change to sim
        if (new_stick_state != 2)
        {
            if (new_stick_state != stick_state)
            {
                int parameter = (control_state == 0) ? 1 : 0;
                SimConnect_TransmitClientEvent(hsimconnect, 0, targets[present_target], parameter,
                    SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                stick_state = new_stick_state;
            }         
        }
        previous_button_press = new_button_press;
        Sleep(10);
    }
    SimConnect_Close(hsimconnect);
    return 0;
}

