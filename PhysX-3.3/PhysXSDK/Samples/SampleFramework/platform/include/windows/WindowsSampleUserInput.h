/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef WINDOWS_SAMPLE_USER_INPUT_H
#define WINDOWS_SAMPLE_USER_INPUT_H


#include <SampleUserInput.h>
#include <windows/WindowsSampleUserInputIds.h>
#include <WTypes.h>
#include <XInput.h>

namespace SampleFramework
{
	class WindowsSampleUserInput: public SampleUserInput
	{
	public:
		enum KeyEventFlag
		{
			KEY_EVENT_NONE = 0,
			KEY_EVENT_UP,
			KEY_EVENT_DOWN,
		};

		enum MouseButtons
		{
			LEFT_MOUSE_BUTTON,
			RIGHT_MOUSE_BUTTON,
			CENTER_MOUSE_BUTTON,
		};

		struct KeyEvent
		{
			WPARAM m_Param;
			USHORT m_ScanCode;
			KeyEventFlag m_Flags;
		};

		struct InputState
		{
			InputState()
			{
			};

			InputState(bool val)
			{
				m_State = val;
			};

			InputState(float val)
			{
				m_Value = val;
			};

			union
			{
				bool	m_State;
				float	m_Value;
			};
		};

		WindowsSampleUserInput();
		~WindowsSampleUserInput();

		bool keyCodeToASCII( WindowsSampleUserInputIds code, char& c )
		{
			if( code >= KEY_A && code <= KEY_Z )
			{
				c = (char)code + 'A' - 1;
			}
			else if( code >= KEY_0 && code <= KEY_9 )
			{
				c = (char)code + '0' - 1;
			}
			else if( code == KEY_SPACE )
			{
				c = ' ';
			}
			else
			{
				return false;
			}
			return true;
		}

		void doOnMouseMove(physx::PxU32 x, physx::PxU32 y, physx::PxReal dx, physx::PxReal dy, WindowsSampleUserInputIds button);
		void doOnMouseButton(physx::PxU32 x, physx::PxU32 y, MouseButtons button, bool down);
		void onKeyDownEx(WPARAM wParam);
		void onKeyDown(WPARAM wParam, LPARAM lParam);
		void onKeyUp(WPARAM wParam, LPARAM lParam);
		void onKeyEvent(const KeyEvent& keyEvent);

		void onGamepadButton(physx::PxU32 buttonIndex, bool buttonDown);
		void onGamepadAnalogButton(physx::PxU32 buttonIndex,const BYTE oldValue,const BYTE newValue);
		void onGamepadAxis(physx::PxU32 axis, physx::PxReal val);

		virtual void updateInput();
		virtual void processGamepads();

		virtual void shutdown();

		virtual bool keyboardSupported() const { return true; }
		virtual bool gamepadSupported() const;
		virtual bool mouseSupported() const { return true; }
		virtual InputType getInputType(const UserInput&) const;

		virtual bool	getDigitalInputEventState(physx::PxU16 inputEventId ) const;
		virtual float	getAnalogInputEventState(physx::PxU16 inputEventId ) const;

	protected:
		WindowsSampleUserInputIds	getKeyCode(WPARAM wParam) const;
		SampleUserInput::KeyCode	getSampleUserInputKeyCode(WPARAM wParam) const;

		const UserInput*			getUserInputFromId(WindowsSampleUserInputIds id) const;

		bool						hasXInput() const { return mpXInputGetState && mpXInputGetCapabilities; }

		std::map<physx::PxU16, physx::PxU16>	mScanCodesMap;
		std::map<physx::PxU16,float>	mAnalogStates;
		std::map<physx::PxU16,bool>		mDigitalStates;

		bool							mGamePadConnected;
		physx::PxU32					mConnectedPad;

		typedef DWORD (WINAPI *LPXINPUTGETSTATE)(DWORD, XINPUT_STATE*);
		typedef DWORD (WINAPI *LPXINPUTGETCAPABILITIES)(DWORD,DWORD,XINPUT_CAPABILITIES*);

		HMODULE							mXInputLibrary;
		LPXINPUTGETSTATE				mpXInputGetState;
		LPXINPUTGETCAPABILITIES			mpXInputGetCapabilities;
	};
}

#endif
