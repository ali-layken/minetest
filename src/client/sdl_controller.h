#include <SDL_events.h>
#include <SDL_gamecontroller.h>
#include <SDL_joystick.h>
#include <IEventReceiver.h>
#include <bitset>

#include <SDL_version.h>
#if SDL_VERSION_ATLEAST(1, 3, 0)
#include <SDL_haptic.h>
#endif

class SDLController
{
private:
	SDL_GameController *m_game_controller;

	SDL_Joystick *m_joystick;

#if SDL_VERSION_ATLEAST(1, 3, 0)
	SDL_Haptic *m_haptic;
#endif

	int m_buttons;

	int m_axes;

	int m_hats;

	SDL_JoystickID m_id;

	irr::SEvent m_irr_event;

	int16_t m_prev_axes[irr::SEvent::SJoystickEvent::NUMBER_OF_AXES];

	uint64_t m_last_power_level_time;
public:
	// ------------------------------------------------------------------------
	SDLController(int device_id);
	// ------------------------------------------------------------------------
	~SDLController();
	// ------------------------------------------------------------------------
	const irr::SEvent &getEvent() const { return m_irr_event; }
	// ------------------------------------------------------------------------
	SDL_JoystickID getInstanceID() const { return m_id; }
	// ------------------------------------------------------------------------
	void handleAxisInputSense(const SDL_Event &event);
	// ------------------------------------------------------------------------
	bool handleAxis(const SDL_Event &event)
	{
		int axis_idx = event.jaxis.axis;
		if (axis_idx > m_axes)
			return false;
		m_irr_event.JoystickEvent.Axis[axis_idx] = event.jaxis.value;
		m_prev_axes[axis_idx] = event.jaxis.value;
		uint32_t value = 1 << axis_idx;
		return true;
	} // handleAxis
	// ------------------------------------------------------------------------
	bool handleHat(const SDL_Event &event)
	{
		if (event.jhat.hat > m_hats)
			return false;
		std::bitset<4> new_hat_status;
		// Up, right, down and left (4 buttons)
		switch (event.jhat.value) {
		case SDL_HAT_UP:
			new_hat_status[0] = true;
			break;
		case SDL_HAT_RIGHTUP:
			new_hat_status[0] = true;
			new_hat_status[1] = true;
			break;
		case SDL_HAT_RIGHT:
			new_hat_status[1] = true;
			break;
		case SDL_HAT_RIGHTDOWN:
			new_hat_status[1] = true;
			new_hat_status[2] = true;
			break;
		case SDL_HAT_DOWN:
			new_hat_status[2] = true;
			break;
		case SDL_HAT_LEFTDOWN:
			new_hat_status[2] = true;
			new_hat_status[3] = true;
			break;
		case SDL_HAT_LEFT:
			new_hat_status[3] = true;
			break;
		case SDL_HAT_LEFTUP:
			new_hat_status[3] = true;
			new_hat_status[0] = true;
			break;
		case SDL_HAT_CENTERED:
		default:
			break;
		}
		int hat_start = m_buttons - (m_hats * 4) + (event.jhat.hat * 4);
		std::bitset<irr::SEvent::SJoystickEvent::NUMBER_OF_BUTTONS> states(
				m_irr_event.JoystickEvent.ButtonStates);
		for (unsigned i = 0; i < 4; i++) {
			int hat_button_id = i + hat_start;
			states[hat_button_id] = new_hat_status[i];
		}
		m_irr_event.JoystickEvent.ButtonStates = (irr::u32)states.to_ulong();
		return true;
	} // handleHat
	// ------------------------------------------------------------------------
	bool handleButton(const SDL_Event &event)
	{
		if (event.jbutton.button > m_buttons) {
#ifdef ANDROID
			handleDirectScanCode(event);
#endif
			return false;
		}
		bool pressed = event.jbutton.state == SDL_PRESSED;
		std::bitset<irr::SEvent::SJoystickEvent::NUMBER_OF_BUTTONS> states(
				m_irr_event.JoystickEvent.ButtonStates);
		states[event.jbutton.button] = pressed;
		m_irr_event.JoystickEvent.ButtonStates = (irr::u32)states.to_ulong();
		return true;
	} // handleButton
	// ------------------------------------------------------------------------
	SDL_GameController *getGameController() const { return m_game_controller; }
	// ------------------------------------------------------------------------
	void doRumble(float strength_low, float strength_high, uint32_t duration_ms);
};
