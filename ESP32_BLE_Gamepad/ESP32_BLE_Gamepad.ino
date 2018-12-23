//	�ȉ����痬�p
//�@https://github.com/nkolban/esp32-snippets/issues/632

#include <Arduino.h>

/**
 * Create a new BLE server.
 */
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"
#include "BLEHIDDevice.h"

static BLEHIDDevice* hid;
BLECharacteristic* input;
BLECharacteristic* output;

class MyCallbacks : public BLEServerCallbacks {
	void onConnect(BLEServer* pServer) {
		Serial.println("connected");

		// workaround after reconnect (see comment below) 
		BLEDescriptor *desc = input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
		uint8_t val[] = { 0x01, 0x00 };
		desc->setValue(val, 2);
	}

	void onDisconnect(BLEServer* pServer) {
		Serial.println("disconnected");
	}
};

void setup() {
	//
	Serial.begin(115200);
	Serial.println("Starting");

	//
	BLEDevice::init("ESP32");

	BLEServer *pServer = BLEDevice::createServer();
	pServer->setCallbacks(new MyCallbacks());

	/*
	 * Instantiate hid device
	 */
	hid = new BLEHIDDevice(pServer);

	input = hid->inputReport(1); // <-- input REPORTID from report map
	output = hid->outputReport(1); // <-- output REPORTID from report map

	/*
	 * Set manufacturer name (OPTIONAL)
	 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.manufacturer_name_string.xml
	 */
	std::string name = "esp-community";
	hid->manufacturer()->setValue(name);

	/*
	 * Set pnp parameters (MANDATORY)
	 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.pnp_id.xml
	 */
	hid->pnp(0x02, 0x0810, 0xe501, 0x0106);

	/*
	 * Set hid informations (MANDATORY)
	 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.hid_information.xml
	 */
	hid->hidInfo(0x00, 0x01);

	/*
	 * Gamepad
	 */
	const uint8_t reportMap[] = {
		0x05, 0x01,  /* USAGE_PAGE (Generic Desktop)       */
		0x09, 0x05,  /* USAGE (Game Pad)                   */
		0xa1, 0x01,  /* COLLECTION (Application)           */
		0xa1, 0x03,  /*   COLLECTION (Report)              */

		0x85, 0x01,  /*     REPORT_ID (1)                  */
		0x05, 0x09,  /*     USAGE_PAGE (Button)            */
		0x19, 0x01,  /*     USAGE_MINIMUM (Button 1)       */
		0x29, 0x0e,  /*     USAGE_MAXIMUM (Button 14)      */
		0x15, 0x00,  /*     LOGICAL_MINIMUM (0)            */
		0x25, 0x01,  /*     LOGICAL_MAXIMUM (1)            */
		0x95, 0x0e,  /*     REPORT_COUNT (14)              */
		0x75, 0x01,  /*     REPORT_SIZE (1)                */
		0x81, 0x02,  /*     INPUT (Data,Var,Abs)           */

		0x95, 0x01,  /*     REPORT_COUNT (1)               */
		0x75, 0x02,  /*     REPORT_SIZE (2)                */	// 1�f�[�^�̃T�C�Y�H
		0x81, 0x03,  /*     INPUT (Cnst)                   */
		0xa1, 0x00,  /*     COLLECTION (Physical)          */
		0x05, 0x01,  /*       USAGE_PAGE (Generic Desktop) */

		0x09, 0x30,  /*       USAGE (X)                    */	// 1��
		0x09, 0x31,  /*       USAGE (Y)                    */	// 2��
		0x09, 0x32,  /*       USAGE (Z)                    */	// 3��

		0x09, 0x33,  /*       USAGE (Rx)                   */	// 4��
		0x09, 0x34,  /*       USAGE (Ry)                   */	// 5��
		0x09, 0x35,  /*       USAGE (Rz)                   */	// 6��

		0x09, 0x36,  /*       (Slider)Slider               */
		0x09, 0x37,  /*       (Dial)Slider                 */
		0x09, 0x38,  /*       (Wheel)Slider                */

		0x15, 0x81,  /*       LOGICAL_MINIMUM (-127)       */	// -127
		0x25, 0x7F,  /*       LOGICAL_MAXIMUM (127)        */	// +127
		0x75, 0x08,  /*       REPORT_SIZE (8)              */	// 1�f�[�^�̃r�b�g��
		0x95, 0x09,  /*       REPORT_COUNT (2)             */	// �f�[�^�̌�
		0x81, 0x02,  /*       INPUT (Data,Var,Abs)         */
		0xc0,        /*     END_COLLECTION                 */
		0xc0,        /*   END_COLLECTION                   */
		0xc0         /* END_COLLECTION                     */
	};

	/*
	 * Set report map (here is initialized device driver on client side) (MANDATORY)
	 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.report_map.xml
	 */
	hid->reportMap((uint8_t*)reportMap, sizeof(reportMap));

	/*
	 * We are prepared to start hid device services. Before this point we can change all values and/or set parameters we need.
	 * Also before we start, if we want to provide battery info, we need to prepare battery service.
	 * We can setup characteristics authorization
	 */
	hid->startServices();

	/*
	 * Its good to setup advertising by providing appearance and advertised service. This will let clients find our device by type
	 */
	BLEAdvertising *pAdvertising = pServer->getAdvertising();
	pAdvertising->setAppearance(HID_GAMEPAD);    // <-- optional
	pAdvertising->addServiceUUID(hid->hidService()->getUUID());
	pAdvertising->start();

	BLESecurity *pSecurity = new BLESecurity();
	pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

	Serial.println("waiting 5sec");
	delay(5000);
}

void lotate(uint8_t x, uint8_t y, uint8_t z, uint8_t x1, uint8_t y1, uint8_t z1)
{
	uint8_t c[] = { 0x00, 0x00, x, y, z, x1, y1, z1,0,0,0};
	input->setValue(c, sizeof(c));
	input->notify();
	delay(50);
}

void loop()
{
	static uint8_t count = 0;

	// put your main code here, to run repeatedly:
	lotate( 64,  64,  0,  0,-50, count);	count++;
	lotate( 90,   0, 10, 80,-60, count);	count++;
	lotate( 64, -64, 20, 90,-70, count);	count++;
	lotate(  0, -90, 30,  0,-80, count);	count++;
	lotate(-64, -64, 40,-10,-90, count);	count++;
	lotate(-90,   0, 50,-20,  0, count);	count++;
	lotate(-64,  64, 60,-30, 10, count);	count++;
	lotate(  0,  90, 70,-40, 20, count);	count++;

}
