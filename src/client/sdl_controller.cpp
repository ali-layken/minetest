#include "sdl_controller.h"
#include "keys.h"
#include "settings.h"
#include "gettime.h"
#include "porting.h"
#include "util/string.h"

#include <SDL_version.h>
#include <stdexcept>
#include <string>

// ----------------------------------------------------------------------------
SDLController::SDLController(int device_id)
{
	m_irr_event = {};
	m_irr_event.EventType = irr::EET_JOYSTICK_INPUT_EVENT;
	memset(m_prev_axes, 0,
			irr::SEvent::SJoystickEvent::NUMBER_OF_AXES * sizeof(int16_t));
	m_game_controller = NULL;
	m_joystick = NULL;
	m_haptic = NULL;
	m_id = -1;

	if (SDL_IsGameController(device_id)) {
		m_game_controller = SDL_GameControllerOpen(device_id);
		if (!m_game_controller)
			throw std::runtime_error(SDL_GetError());
		m_joystick = SDL_GameControllerGetJoystick(m_game_controller);
		if (!m_joystick) {
			SDL_GameControllerClose(m_game_controller);
			throw std::runtime_error(SDL_GetError());
		}
	} else {
		m_joystick = SDL_JoystickOpen(device_id);
		if (!m_joystick)
			throw std::runtime_error(SDL_GetError());
	}

	m_id = SDL_JoystickInstanceID(m_joystick);
	if (m_id < 0) {
		if (m_game_controller)
			SDL_GameControllerClose(m_game_controller);
		else
			SDL_JoystickClose(m_joystick);
		throw std::runtime_error(SDL_GetError());
	}

	m_irr_event.JoystickEvent.Joystick = m_id;
	const char *name_cstr = SDL_JoystickName(m_joystick);
	if (name_cstr == NULL) {
		if (m_game_controller)
			SDL_GameControllerClose(m_game_controller);
		else
			SDL_JoystickClose(m_joystick);
		throw std::runtime_error("missing name for joystick");
	}
	std::string name = name_cstr;
#ifdef WIN32
	// SDL added #number to xinput controller which is its user id, we remove
	// it manually to allow hotplugging to get the same config each time
	// user id ranges from 0-3
	// From GetXInputName(const Uint8 userid, BYTE SubType) in
	// SDL_xinputjoystick.c
	if ((name.size() > 7 && name.compare(0, 7, "XInput ") == 0 &&
			    name[name.size() - 2] == '#') ||
			(name.size() > 16 &&
					name.compare(0, 16, "X360 Controller ") == 0 &&
					name[name.size() - 2] == '#'))
		name.erase(name.length() - 3);
#endif

	m_buttons = SDL_JoystickNumButtons(m_joystick);
	if (m_buttons < 0) {
		if (m_game_controller)
			SDL_GameControllerClose(m_game_controller);
		else
			SDL_JoystickClose(m_joystick);
		throw std::runtime_error(SDL_GetError());
	}

	m_axes = SDL_JoystickNumAxes(m_joystick);
	if (m_axes < 0) {
		if (m_game_controller)
			SDL_GameControllerClose(m_game_controller);
		else
			SDL_JoystickClose(m_joystick);
		throw std::runtime_error(SDL_GetError());
	}

	m_hats = SDL_JoystickNumHats(m_joystick);
	if (m_hats < 0) {
		if (m_game_controller)
			SDL_GameControllerClose(m_game_controller);
		else
			SDL_JoystickClose(m_joystick);
		throw std::runtime_error(SDL_GetError());
	}

	if (m_buttons > irr::SEvent::SJoystickEvent::NUMBER_OF_BUTTONS)
		m_buttons = irr::SEvent::SJoystickEvent::NUMBER_OF_BUTTONS;
	if (m_axes > irr::SEvent::SJoystickEvent::NUMBER_OF_AXES)
		m_axes = irr::SEvent::SJoystickEvent::NUMBER_OF_AXES;
	// We store hats event with 4 buttons
	int max_buttons_with_hats =
			irr::SEvent::SJoystickEvent::NUMBER_OF_BUTTONS - (m_hats * 4);
	if (m_buttons > max_buttons_with_hats)
		m_hats = 0;
	else
		m_buttons += m_hats * 4;
	// Save previous axes values for input sensing
	for (int i = 0; i < m_axes; i++)
		m_prev_axes[i] = SDL_JoystickGetAxis(m_joystick, i);

	std::string mapping_string;
	if (m_game_controller) {
		char *mapping = SDL_GameControllerMapping(m_game_controller);
		if (mapping) {
			mapping_string = mapping;
			SDL_free(mapping);
		}
	}

#if SDL_VERSION_ATLEAST(1, 3, 0)
	m_haptic = SDL_HapticOpenFromJoystick(m_joystick);
	if (m_haptic)
		SDL_HapticRumbleInit(m_haptic);
#endif

} // SDLController

// ----------------------------------------------------------------------------
SDLController::~SDLController()
{
	if (m_game_controller)
		SDL_GameControllerClose(m_game_controller);
	else
		SDL_JoystickClose(m_joystick);
#if SDL_VERSION_ATLEAST(1, 3, 0)
	if (m_haptic)
		SDL_HapticClose(m_haptic);
#endif
} // ~SDLController

// ----------------------------------------------------------------------------
/** SDL only sends event when axis moves, so we need to send previously saved
 *  event for correct input sensing. */
void SDLController::handleAxisInputSense(const SDL_Event &event)
{
	int axis_idx = event.jaxis.axis;
	if (axis_idx > m_axes)
		return;
	if (event.jaxis.value == m_prev_axes[axis_idx])
		return;
} // handleAxisInputSense

void SDLController::doRumble(
		float strength_low, float strength_high, uint32_t duration_ms)
{
#if SDL_VERSION_ATLEAST(1, 3, 0)
	if (m_haptic) {
		SDL_HapticRumblePlay(m_haptic, (strength_low + strength_high) / 2,
				duration_ms);
	} else
#endif
	{
#if SDL_VERSION_ATLEAST(2, 0, 9)
		uint16_t scaled_low = strength_low * pow(2, 16);
		uint16_t scaled_high = strength_high * pow(2, 16);
		SDL_GameControllerRumble(
				m_game_controller, scaled_low, scaled_high, duration_ms);
#endif
	}
}
