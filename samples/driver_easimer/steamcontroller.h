// === Copyright (c) 2017-2020 easimer.net. All rights reserved. ===

#include "steam_controller/steamcontroller.h"

class CSteamController {
public:
	CSteamController() : m_pDevice(NULL) {
	}

	CSteamController(const SteamControllerDeviceEnum* pDeviceEnum)
		: m_pDevice(SteamController_Open(pDeviceEnum)) {
	}

	~CSteamController() {
		if (m_pDevice) {
			SteamController_Close(m_pDevice);
		}
	}

	bool IsWirelessDongle() {
		return SteamController_IsWirelessDongle(m_pDevice);
	}

	bool TurnOff() {
		return SteamController_TurnOff(m_pDevice);
	}

	bool QueryWirelessState(uint8_t* state) {
		return SteamController_QueryWirelessState(m_pDevice, state);
	}

	bool EnablePairing(bool enable, uint8_t deviceType) {
		return SteamController_EnablePairing(m_pDevice, enable, deviceType);
	}

	bool CommitPairing(bool connect) {
		return SteamController_CommitPairing(m_pDevice, connect);
	}

	bool Configure(unsigned configFlags) {
		return SteamController_Configure(m_pDevice, configFlags);
	}

	bool SetHomeButtonBrightness(uint8_t brightness) {
		return SteamController_SetHomeButtonBrightness(m_pDevice, brightness);
	}

	bool SetTimeOut(uint16_t timeout) {
		return SteamController_SetTimeOut(m_pDevice, timeout);
	}

	bool TriggerHaptic(uint16_t motor, uint16_t onTime, uint16_t offTime, uint16_t count) {
		return SteamController_TriggerHaptic(m_pDevice, motor, onTime, offTime, count);
	}

	void PlayMelody(uint32_t melody) {
		SteamController_PlayMelody(m_pDevice, melody);
	}

	virtual void OnUpdate(const SteamControllerUpdateEvent& ev) {}
	virtual void OnDisconnect() {}
	virtual void OnBattery(uint16_t voltage) {}

	void RunFrames() {
		SteamControllerEvent ev;
		if (m_pDevice != NULL) {
			if (SteamController_ReadEvent(m_pDevice, &ev)) {
				if (ev.eventType == STEAMCONTROLLER_EVENT_UPDATE) {
					OnUpdate(ev.update);
				}
				else if (ev.eventType == STEAMCONTROLLER_EVENT_CONNECTION) {
					if (ev.connection.details == 1) {
						SteamController_Close(m_pDevice);
						m_pDevice = NULL;
						OnDisconnect();
					}
				}
			}
		}
	}

	bool IsConnected() const { return m_pDevice; }
private:
	SteamControllerDevice* m_pDevice;
};
