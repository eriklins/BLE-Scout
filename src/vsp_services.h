typedef const struct _vsp_characteristic_t VspCharacteristic;
const struct _vsp_characteristic_t
{
    const char* uuid;
    const char* name;
};

typedef const struct _vsp_service_t VspService;
const struct _vsp_service_t
{
    const char* uuid;
    const char* name;
    VspCharacteristic Rx;
    VspCharacteristic Tx;
    VspCharacteristic ModemIn;
    VspCharacteristic ModemOut;
};

const uint32_t vsp_service_uuids_len = 3;
VspService vsp_service_uuids[3] = {
    {"569a1101-b87f-490c-92cb-11ba5ea5167c", "Laird Connectivity VSP Service",
        {"569a2001-b87f-490c-92cb-11ba5ea5167c", "Laird Vsp Rx"},
        {"569a2000-b87f-490c-92cb-11ba5ea5167c", "Laird Vsp Tx"},
        {"569a2003-b87f-490c-92cb-11ba5ea5167c", "Laird Vsp ModemIn"},
        {"569a2002-b87f-490c-92cb-11ba5ea5167c", "Laird Vsp ModemOut"}
    },
    {"6e400001-b5a3-f393-e0a9-e50e24dcca9e", "Nordic UART Service (NUS)",
        {"6e400002-b5a3-f393-e0a9-e50e24dcca9e", "NUS Rx"},
        {"6e400003-b5a3-f393-e0a9-e50e24dcca9e", "NUS Tx"},
        {"", ""},
        {"", ""}
    },
    {"2456e1b9-26e2-8f83-e744-f34f01e9d701", "u-connectXpress BLE Serial Port Service",
        {"2456e1b9-26e2-8f83-e744-f34f01e9d703", "Serial Port FIFO / Rx"},
        {"2456e1b9-26e2-8f83-e744-f34f01e9d704", "Serial Port Credit / Tx"},
        {"", ""},
        {"", ""}
    }
};
