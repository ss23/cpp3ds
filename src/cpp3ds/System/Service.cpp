#include <cpp3ds/Config.hpp>
#include <cpp3ds/System/Service.hpp>
#include <cpp3ds/System/Err.hpp>
#include <3ds.h>
#include <malloc.h>
#include <sys/unistd.h>
#include <string.h>

namespace cpp3ds {

Uint16 Service::m_enabledServices = 0x0;
u32*   Service::m_socBuffer       = nullptr;
u8*    Service::m_micBuffer       = nullptr;


bool Service::enable(ServiceName service) {
	if (isEnabled(service))
		return true;

	bool success = false;
	Result result;

	switch (service) {
		case All:
			return enable(Network) && enable(Audio) &&
			       enable(Config) && enable(RomFS) && enable(Microphone);
		case Network:
			m_socBuffer = (u32*) memalign(0x1000, 0x100000);
			if (m_socBuffer == nullptr)
				break;
			result = socInit(m_socBuffer, 0x100000);
			if (result != 0) {
				err() << "Network service (soc:U) failed to initialize: " << std::hex << result;
				socExit();
				free(m_socBuffer);
				break;
			}
			success = true;
			break;
		case Audio:
			result = ndspInit();
			success = R_SUCCEEDED(result);
			if (!success)
				err() << "Audio service (dsp) failed to initialize: " << std::hex << result;
			break;
		case Config:
			success = R_SUCCEEDED(cfguInit());
			break;
		case RomFS:
			success = R_SUCCEEDED(romfsInit());
			chdir("romfs:/");
			break;
		case WifiStatus:
			success = R_SUCCEEDED(acInit());
			break;
		case Microphone:
			if (!enable(Audio))
				break;
			m_micBuffer = (u8*) memalign(0x1000, 0x30000);
			if (m_micBuffer == nullptr)
				break;
			success = R_SUCCEEDED(micInit(m_micBuffer, 0x30000));
			break;
		default:
			break;
	}

	if (success)
		m_enabledServices |= service;
	return success;
}


bool Service::disable(ServiceName service) {
	if (service == All)
		return disable(Network) && disable(Audio) &&
			   disable(Config) && disable(RomFS) && disable(Microphone);

	if (!isEnabled(service))
		return true;

	bool success = true;

	switch (service) {
		case Network:
			socExit();
			free(m_socBuffer);
			break;
		case Audio:
			ndspExit();
			break;
		case Config:
			cfguExit();
			break;
		case RomFS:
			romfsExit();
			break;
		case WifiStatus:
			acExit();
			break;
		case Microphone:
			micExit();
			free(m_micBuffer);
			break;
		default:
			break;
	}

	if (success)
		m_enabledServices &= ~service;
	return success;
}


bool Service::isEnabled(ServiceName service) {
	if (service == Network) {
		if (!enable(WifiStatus))
			return false;
		u32 status;
		Result result = ACU_GetWifiStatus(&status);
		if (result != 0 || status == 0)
			return false;
	}
	return service & m_enabledServices;
}

} // namespace cpp3ds
