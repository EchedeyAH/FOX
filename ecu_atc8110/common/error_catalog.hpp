#pragma once
/**
 * error_catalog.hpp - Catálogo oficial de códigos de error para ECU ATC8110
 * ÚNICA FUENTE DE VERDAD para códigos de error.
 * Basado en sistema legacy ECU_FOX_rc30 con categorías HMI.
 */

#include <cstdint>
#include <vector>

namespace ecu {

// Enums oficiales según especificación HMI
enum class ErrorLevel : uint8_t { INFORMATIVO = 0, LEVE = 1, GRAVE = 2, CRITICO = 3 };
enum class ErrorGroup : uint8_t { BMS = 0, PILA = 1, CONTROL = 2, MOTOR = 3, CAN = 4, ERROR = 5, SENSORES = 6, IMU = 7 };
enum class ErrorAction : uint8_t { NONE = 0, LOG = 1, LIMP_MODE = 2, TORQUE_0 = 3, EMERGENCY_STOP = 4 };

// Códigos de error (ID estable 0xGGNN)
enum class ErrorCode : uint16_t {
    BMS_COM_LOST = 0x0001, BMS_TEMP_HIGH = 0x0002, BMS_VOLT_HIGH = 0x0003, BMS_VOLT_LOW = 0x0004,
    BMS_CURR_HIGH = 0x0005, BMS_CHASIS_CONN = 0x0006, BMS_CELL_COM_ERR = 0x0007, BMS_SYS_ERR = 0x0008,
    BMS_SOC_LOW = 0x0009, BMS_SOC_CRITICAL = 0x000A,
    PILA_VOLT_DIFF = 0x1001, PILA_TEMP_DIFF = 0x1002, PILA_CELL_LOW = 0x1003, PILA_CELL_HIGH = 0x1004,
    CTRL_HILOS_FAIL = 0x2001, CTRL_TAD_AO_FAIL = 0x2002, CTRL_TAD_FAIL = 0x2003, CTRL_FSM_ERR = 0x2004, CTRL_WATCHDOG = 0x2005,
    M1_COM_TIMEOUT = 0x3001, M1_VOLT_HIGH = 0x3002, M1_VOLT_LOW = 0x3003, M1_TEMP_HIGH = 0x3004, M1_DIFF_PWR = 0x3005, M1_VAUX_ERR = 0x3006, M1_CONFIG_ERR = 0x3007,
    M2_COM_TIMEOUT = 0x4001, M2_VOLT_HIGH = 0x4002, M2_VOLT_LOW = 0x4003, M2_TEMP_HIGH = 0x4004, M2_DIFF_PWR = 0x4005, M2_VAUX_ERR = 0x4006, M2_CONFIG_ERR = 0x4007,
    M3_COM_TIMEOUT = 0x5001, M3_VOLT_HIGH = 0x5002, M3_VOLT_LOW = 0x5003, M3_TEMP_HIGH = 0x5004, M3_DIFF_PWR = 0x5005, M3_VAUX_ERR = 0x5006, M3_CONFIG_ERR = 0x5007,
    M4_COM_TIMEOUT = 0x6001, M4_VOLT_HIGH = 0x6002, M4_VOLT_LOW = 0x6003, M4_TEMP_HIGH = 0x6004, M4_DIFF_PWR = 0x6005, M4_VAUX_ERR = 0x6006, M4_CONFIG_ERR = 0x6007,
    CAN_TX_FAIL = 0x7001, CAN_RX_TIMEOUT = 0x7002, CAN_FRAME_INVALID = 0x7003, CAN_BUS_OFF = 0x7004, CAN_SUPERV_TIMEOUT = 0x7005, CAN_SUPERV_OFF = 0x7006, CAN_OVERLOAD = 0x7007,
    ERR_EMERGENCY = 0x8001, ERR_INTERNAL = 0x8002, ERR_MEMORY = 0x8003, ERR_HW_FAULT = 0x8004,
    SENS_SUSP_DD_ERR = 0x9001, SENS_SUSP_DI_ERR = 0x9002, SENS_VOLANT_ERR = 0x9005, SENS_FRENO_ERR = 0x9006, SENS_ACEL_ERR = 0x9007, SENS_RANGE_ERR = 0x9008,
    IMU_COM_ERR = 0xA001, IMU_DATA_ERR = 0xA002, IMU_CALIB_ERR = 0xA003,
};

// Estructura de entrada del catálogo
struct ErrorCatalogEntry {
    ErrorCode code; ErrorLevel level; ErrorGroup group; const char* origin;
    const char* description; const char* trigger_condition; const char* resolve_condition;
    ErrorAction action; uint32_t timeout_ms; int32_t threshold_value; const char* threshold_unit;
};

// Funciones helper para crear entradas
inline ErrorCatalogEntry make_err(uint16_t code, uint8_t level, uint8_t group, const char* orig, const char* desc, 
    const char* trig, const char* res, uint8_t action, uint32_t to, int32_t thresh, const char* unit) {
    return {(ErrorCode)code, (ErrorLevel)level, (ErrorGroup)group, orig, desc, trig, res, (ErrorAction)action, to, thresh, unit};
}

inline std::vector<ErrorCatalogEntry> get_error_catalog() {
    std::vector<ErrorCatalogEntry> v;
    // BMS
    v.push_back(make_err(0x0001,1,0,"BMS","BMS comm lost","No msg BMS>1s","Recibir",2,1000,0,"ms"));
    v.push_back(make_err(0x0002,2,0,"BMS","BMS temp alta","Temp>65°C","Temp<50°C",2,0,65,"°C"));
    v.push_back(make_err(0x0003,2,0,"BMS","BMS volt alto","Vpack>95V","Vpack<90V",2,0,95000,"mV"));
    v.push_back(make_err(0x0004,2,0,"BMS","BMS volt bajo","Vpack<50V","Vpack>60V",2,0,50000,"mV"));
    v.push_back(make_err(0x0005,1,0,"BMS","BMS curr alta","I>limite","I normal",0,0,0,"A"));
    v.push_back(make_err(0x0006,3,0,"BMS","BMS chassis","Conectado","Desconectado",4,0,0,"-"));
    v.push_back(make_err(0x0007,2,0,"BMS","BMS cell comm","Error celdas","OK",2,0,0,"-"));
    v.push_back(make_err(0x0008,3,0,"BMS","BMS sys err","Error sistema","OK",4,0,0,"-"));
    v.push_back(make_err(0x0009,1,0,"BMS","BMS SOC bajo","SOC<20%","SOC>25%",2,0,20,"%"));
    v.push_back(make_err(0x000A,2,0,"BMS","BMS SOC crit","SOC<10%","SOC>15%",3,0,10,"%"));
    // CONTROL
    v.push_back(make_err(0x2001,3,2,"CTRL","Hilos fail","No arr","OK",4,0,0,"-"));
    v.push_back(make_err(0x2002,2,2,"CTRL","TAD AO fail","No responde","OK",2,0,0,"-"));
    v.push_back(make_err(0x2003,2,2,"CTRL","TAD fail","No responde","OK",2,0,0,"-"));
    v.push_back(make_err(0x2004,2,2,"CTRL","FSM error","Estado inválido","OK",2,0,0,"-"));
    v.push_back(make_err(0x2005,3,2,"CTRL","Watchdog","Expirado","Reset",4,500,0,"ms"));
    // MOTOR 1
    v.push_back(make_err(0x3001,3,3,"M1","M1 timeout","No msg>2s","Recibir",4,2000,0,"ms"));
    v.push_back(make_err(0x3002,2,3,"M1","M1 volt alto","V>90V","V<85V",2,0,90,"V"));
    v.push_back(make_err(0x3003,2,3,"M1","M1 volt bajo","V<60V","V>65V",2,0,60,"V"));
    v.push_back(make_err(0x3004,2,3,"M1","M1 temp alta","Temp>80°C","Temp<60°C",3,0,80,"°C"));
    v.push_back(make_err(0x3005,2,3,"M1","M1 diff pwr","Diff>umbral","Normal",2,0,0,"W"));
    v.push_back(make_err(0x3006,1,3,"M1","M1 Vaux err","Vaux!=(4-6V)","OK",2,0,0,"V"));
    v.push_back(make_err(0x3007,2,3,"M1","M1 config","Inválida","OK",2,0,0,"-"));
    // MOTOR 2
    v.push_back(make_err(0x4001,3,3,"M2","M2 timeout","No msg>2s","Recibir",4,2000,0,"ms"));
    v.push_back(make_err(0x4002,2,3,"M2","M2 volt alto","V>90V","V<85V",2,0,90,"V"));
    v.push_back(make_err(0x4003,2,3,"M2","M2 volt bajo","V<60V","V>65V",2,0,60,"V"));
    v.push_back(make_err(0x4004,2,3,"M2","M2 temp alta","Temp>80°C","Temp<60°C",3,0,80,"°C"));
    v.push_back(make_err(0x4005,2,3,"M2","M2 diff pwr","Diff>umbral","Normal",2,0,0,"W"));
    v.push_back(make_err(0x4006,1,3,"M2","M2 Vaux err","Vaux!=(4-6V)","OK",2,0,0,"V"));
    v.push_back(make_err(0x4007,2,3,"M2","M2 config","Inválida","OK",2,0,0,"-"));
    // MOTOR 3
    v.push_back(make_err(0x5001,3,3,"M3","M3 timeout","No msg>2s","Recibir",4,2000,0,"ms"));
    v.push_back(make_err(0x5002,2,3,"M3","M3 volt alto","V>90V","V<85V",2,0,90,"V"));
    v.push_back(make_err(0x5003,2,3,"M3","M3 volt bajo","V<60V","V>65V",2,0,60,"V"));
    v.push_back(make_err(0x5004,2,3,"M3","M3 temp alta","Temp>80°C","Temp<60°C",3,0,80,"°C"));
    v.push_back(make_err(0x5005,2,3,"M3","M3 diff pwr","Diff>umbral","Normal",2,0,0,"W"));
    v.push_back(make_err(0x5006,1,3,"M3","M3 Vaux err","Vaux!=(4-6V)","OK",2,0,0,"V"));
    v.push_back(make_err(0x5007,2,3,"M3","M3 config","Inválida","OK",2,0,0,"-"));
    // MOTOR 4
    v.push_back(make_err(0x6001,3,3,"M4","M4 timeout","No msg>2s","Recibir",4,2000,0,"ms"));
    v.push_back(make_err(0x6002,2,3,"M4","M4 volt alto","V>90V","V<85V",2,0,90,"V"));
    v.push_back(make_err(0x6003,2,3,"M4","M4 volt bajo","V<60V","V>65V",2,0,60,"V"));
    v.push_back(make_err(0x6004,2,3,"M4","M4 temp alta","Temp>80°C","Temp<60°C",3,0,80,"°C"));
    v.push_back(make_err(0x6005,2,3,"M4","M4 diff pwr","Diff>umbral","Normal",2,0,0,"W"));
    v.push_back(make_err(0x6006,1,3,"M4","M4 Vaux err","Vaux!=(4-6V)","OK",2,0,0,"V"));
    v.push_back(make_err(0x6007,2,3,"M4","M4 config","Inválida","OK",2,0,0,"-"));
    // CAN
    v.push_back(make_err(0x7001,2,4,"CAN","CAN TX fail","Error TX","OK",2,0,0,"-"));
    v.push_back(make_err(0x7002,2,4,"CAN","CAN RX timeout","No msg>timeout","Recibir",2,1000,0,"ms"));
    v.push_back(make_err(0x7003,1,4,"CAN","CAN frame inv","Frame error","OK",0,0,0,"-"));
    v.push_back(make_err(0x7004,3,4,"CAN","CAN bus off","Bus off","Recovery",4,0,0,"-"));
    v.push_back(make_err(0x7005,3,4,"SUP","SUP timeout","No HB>500ms","Recibir HB",4,500,0,"ms"));
    v.push_back(make_err(0x7006,3,4,"SUP","SUP off","Cmd off","Cmd on",4,0,0,"-"));
    v.push_back(make_err(0x7007,2,4,"CAN","CAN overload","Bus saturado","Normal",2,0,0,"-"));
    // ERROR
    v.push_back(make_err(0x8001,3,5,"SYS","Emergency","Btn activo","Reset",4,0,0,"-"));
    v.push_back(make_err(0x8002,3,5,"SYS","Internal err","Error interno","Reset",4,0,0,"-"));
    v.push_back(make_err(0x8003,3,5,"SYS","Memory err","Error mem","OK",4,0,0,"-"));
    v.push_back(make_err(0x8004,3,5,"SYS","HW fault","Error HW","HW OK",4,0,0,"-"));
    // SENSORES
    v.push_back(make_err(0x9001,1,6,"SUSP","Susp DD err","DD outlier","OK",0,0,0,"-"));
    v.push_back(make_err(0x9002,1,6,"SUSP","Susp DI err","DI outlier","OK",0,0,0,"-"));
    v.push_back(make_err(0x9005,1,6,"VOL","Volant err","Volant outlier","OK",0,0,0,"-"));
    v.push_back(make_err(0x9006,2,6,"FRENO","Freno err","Freno outlier","OK",2,0,0,"-"));
    v.push_back(make_err(0x9007,2,6,"ACEL","Acel err","Acel outlier","OK",2,0,0,"-"));
    v.push_back(make_err(0x9008,2,6,"SENS","Range err","Out of range","In range",2,0,0,"-"));
    // IMU
    v.push_back(make_err(0xA001,2,7,"IMU","IMU comm err","No responde","OK",2,0,0,"-"));
    v.push_back(make_err(0xA002,1,7,"IMU","IMU data err","Datos corruptos","OK",0,0,0,"-"));
    return v;
}

inline const ErrorCatalogEntry* get_error_entry(ErrorCode code) {
    static auto catalog = get_error_catalog();
    for (const auto& e : catalog) { if (e.code == code) return &e; }
    return nullptr;
}

inline const char* error_level_to_string(ErrorLevel l) {
    if (l == ErrorLevel::INFORMATIVO) return "INFORMATIVO";
    if (l == ErrorLevel::LEVE) return "LEVE";
    if (l == ErrorLevel::GRAVE) return "GRAVE";
    if (l == ErrorLevel::CRITICO) return "CRITICO";
    return "DESCONOCIDO";
}
inline const char* error_group_to_string(ErrorGroup g) {
    if (g == ErrorGroup::BMS) return "BMS";
    if (g == ErrorGroup::PILA) return "PILA";
    if (g == ErrorGroup::CONTROL) return "CONTROL";
    if (g == ErrorGroup::MOTOR) return "MOTOR";
    if (g == ErrorGroup::CAN) return "CAN";
    if (g == ErrorGroup::ERROR) return "ERROR";
    if (g == ErrorGroup::SENSORES) return "SENSORES";
    if (g == ErrorGroup::IMU) return "IMU";
    return "DESCONOCIDO";
}
inline const char* error_action_to_string(ErrorAction a) {
    if (a == ErrorAction::NONE) return "NONE";
    if (a == ErrorAction::LOG) return "LOG";
    if (a == ErrorAction::LIMP_MODE) return "LIMP_MODE";
    if (a == ErrorAction::TORQUE_0) return "TORQUE_0";
    if (a == ErrorAction::EMERGENCY_STOP) return "EMERGENCY_STOP";
    return "DESCONOCIDO";
}

} // namespace ecu
