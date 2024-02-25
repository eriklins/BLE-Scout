/* Utilities */

#include "util.hxx"


String* util_vsp_service_uuid_to_name(const char_t* uuid);

String* util_vsp_characteristic_uuid_to_name(const char_t* uuid);

String* util_service_uuid_to_name(const char_t* uuid);

String* util_characteristic_uuid_to_name(const char_t* uuid);

String* util_descriptor_uuid_to_name(const char_t* uuid);

String* util_company_id_to_name(const uint16_t id);

String* util_gap_appearance_to_name(const uint16_t catval);

void util_data_to_hex(const byte_t* in, char* out, const size_t len);

void util_hex_to_data(const char* in, byte_t* out, const size_t len);

void util_data_to_ascii(const byte_t* in, char* out, const size_t len);

bool_t util_buffer_is_hex(const char* in, const size_t len);

bool_t util_buffer_is_mac_address(const char* in, const size_t len);

void util_message_window(Window* window, const char* message);

Util* util_create(void);

void util_destroy(Util** u);
