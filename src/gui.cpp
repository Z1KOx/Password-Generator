#include "gui.h"
#include "generator.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"
#include <string>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter
);

long __stdcall WindowProcess(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter)
{
	if (ImGui_ImplWin32_WndProcHandler(window, message, wideParameter, longParameter))
		return true;

	switch (message)
	{
	case WM_SIZE: {
		if (gui::device && wideParameter != SIZE_MINIMIZED)
		{
			gui::presentParameters.BackBufferWidth = LOWORD(longParameter);
			gui::presentParameters.BackBufferHeight = HIWORD(longParameter);
			gui::ResetDevice();
		}
	}return 0;

	case WM_SYSCOMMAND: {
		if ((wideParameter & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
	}break;

	case WM_DESTROY: {
		PostQuitMessage(0);
	}return 0;

	case WM_LBUTTONDOWN: {
		gui::position = MAKEPOINTS(longParameter); // set click points
	}return 0;

	case WM_MOUSEMOVE: {
		if (wideParameter == MK_LBUTTON)
		{
			const auto points = MAKEPOINTS(longParameter);
			auto rect = ::RECT{ };

			GetWindowRect(gui::window, &rect);

			rect.left += points.x - gui::position.x;
			rect.top += points.y - gui::position.y;

			if (gui::position.x >= 0 &&
				gui::position.x <= gui::WIDTH &&
				gui::position.y >= 0 && gui::position.y <= 19)
				SetWindowPos(
					gui::window,
					HWND_TOPMOST,
					rect.left,
					rect.top,
					0, 0,
					SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER
				);
		}

	}return 0;

	}

	return DefWindowProc(window, message, wideParameter, longParameter);
}

void gui::CreateHWindow(const char* windowName) noexcept
{
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WindowProcess;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandleA(0);
	windowClass.hIcon = 0;
	windowClass.hCursor = 0;
	windowClass.hbrBackground = 0;
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = "class001";
	windowClass.hIconSm = 0;

	RegisterClassEx(&windowClass);

	int windowPosX = (GetSystemMetrics(SM_CXSCREEN) - WIDTH) / 2;
	int windowPosY = (GetSystemMetrics(SM_CYSCREEN) - HEIGHT) / 2;

	window = CreateWindowEx(
		0,
		"class001",
		windowName,
		WS_POPUP,
		windowPosX,
		windowPosY,
		WIDTH,
		HEIGHT,
		0,
		0,
		windowClass.hInstance,
		0
	);

	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);
}

void gui::DestroyHWindow() noexcept
{
	DestroyWindow(window);
	UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
}

bool gui::CreateDevice() noexcept
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3d)
		return false;

	ZeroMemory(&presentParameters, sizeof(presentParameters));

	presentParameters.Windowed = TRUE;
	presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
	presentParameters.EnableAutoDepthStencil = TRUE;
	presentParameters.AutoDepthStencilFormat = D3DFMT_D16;
	presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (d3d->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		window,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&presentParameters,
		&device) < 0)
		return false;

	return true;
}

void gui::ResetDevice() noexcept
{
	ImGui_ImplDX9_InvalidateDeviceObjects();

	const auto result = device->Reset(&presentParameters);

	if (result == D3DERR_INVALIDCALL)
		IM_ASSERT(0);

	ImGui_ImplDX9_CreateDeviceObjects();
}

void gui::DestroyDevice() noexcept
{
	if (device)
	{
		device->Release();
		device = nullptr;
	}

	if (d3d)
	{
		d3d->Release();
		d3d = nullptr;
	}
}

void gui::CreateImGui() noexcept
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ::ImGui::GetIO();

	io.IniFilename = NULL;

	ImGuiStyle& style = ImGui::GetStyle();
	style.GrabRounding = 5.f;
	style.GrabMinSize = 8.f;
	style.ScrollbarSize = 10.f;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(device);
}

void gui::DestroyImGui() noexcept
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void gui::BeginRender() noexcept
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);

		if (message.message == WM_QUIT)
		{
			isRunning = !isRunning;
			return;
		}
	}

	// Start the Dear ImGui frame
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void gui::EndRender() noexcept
{
	ImGui::EndFrame();

	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);

	if (device->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		device->EndScene();
	}

	const auto result = device->Present(0, 0, 0, 0);

	// Handle loss of D3D9 device
	if (result == D3DERR_DEVICELOST && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		ResetDevice();
}

void logicButtons(int index, std::vector<std::string>& generatedPasswords, std::vector<std::string>& passwordNames, std::vector<bool>& deleteButtonClicked)
{
	ImVec2 buttonSize(10.f, 10.f);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(ImColor(117, 32, 32, 255)));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(ImColor(191, 50, 50, 255)));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(ImColor(247, 77, 77, 255)));

	if (!deleteButtonClicked[index] && ImGui::Button(("##Delete" + std::to_string(index)).c_str(), buttonSize))
	{
		deleteButtonClicked[index] = true;

		if (index < generatedPasswords.size())
		{
			generatedPasswords.erase(generatedPasswords.begin() + index);
			passwordNames.erase(passwordNames.begin() + index);
		}
	}

	ImGui::PopStyleColor(3);

	// For Tooltip [Delete] [Copy]
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20.f);

	// Tooltip Delete
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(ImColor(200, 70, 70, 255)));
	ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(ImColor(86, 36, 37, 255)));

	if (ImGui::IsItemHovered() && !deleteButtonClicked[index])
		ImGui::SetTooltip("Delete");

	ImGui::PopStyleColor(2);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(ImColor(32, 32, 117, 255)));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(ImColor(50, 50, 191, 255)));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(ImColor(77, 77, 247, 255)));

	if (!deleteButtonClicked[index])
	{
		ImGui::SameLine();

		if (ImGui::Button(("##Copy" + std::to_string(index)).c_str(), buttonSize))
			ImGui::SetClipboardText(generatedPasswords[index].c_str());

		// Tooltip Copy
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(ImColor(70, 70, 200, 255)));
		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(ImColor(21, 25, 66, 255)));

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Copy");

		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar(1);
	}

	ImGui::PopStyleColor(3);
}

void gui::Render() noexcept
{
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ WIDTH, HEIGHT });

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	ImGui::Begin(
		"Password Generator Window",
		&isRunning,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoDecoration
	);

	// Title bar
	{
		ImGui::SetCursorPos({ 0.f, 5.f });

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(ImColor(28, 28, 29, 255)));
		ImGui::BeginChild("TitleBar", ImVec2(471.f, 20.f));
		{
			ImGui::SetCursorPos({ 11.f, 3.f });
			ImGui::Text("Password Generator");

			ImGui::SameLine();
			ImGui::SetCursorPosX(410.f);
			ImGui::Text("1.1.1.1");

			ImGui::PopStyleColor(1);
			ImGui::EndChild();
		}
	}

	// Title bar line
	{
		ImGui::SetCursorPos({ 0.f, 25.f });
		ImGui::BeginChild("TitleBar_Line", ImVec2(471.f, 1.f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs);
		{
			ImDrawList* draw = ImGui::GetWindowDrawList();

			const ImVec2 gradient_start = ImGui::GetCursorScreenPos();
			const ImVec2 gradient_end = ImVec2(gradient_start.x + 495.f, gradient_start.y + 1.f);
			const ImVec4 col_start = ImVec4(ImColor(160, 93, 116, 255));
			const ImVec4 col_end = ImVec4(ImColor(104, 71, 113, 255));

			constexpr int num_steps = 100;
			constexpr float section_width = 495.f / num_steps;

			for (int i = 0; i < num_steps; ++i)
			{
				const ImVec2 rect_start(gradient_start.x + i * section_width, gradient_start.y);
				const ImVec2 rect_end(gradient_start.x + (i + 1) * section_width, gradient_end.y);

				const float t = static_cast<float>(i) / num_steps;
				const ImVec4 col = ImVec4(
					col_start.x + (col_end.x - col_start.x) * t,
					col_start.y + (col_end.y - col_start.y) * t,
					col_start.z + (col_end.z - col_start.z) * t,
					col_start.w + (col_end.w - col_start.w) * t
				);

				draw->AddRectFilled(rect_start, rect_end, ImGui::ColorConvertFloat4ToU32(col));
			}

			ImGui::EndChild();
		}
	}

	// Exit button
	{
		ImGui::SetCursorPos(ImVec2(480.f, 10.f));

		// Button color
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(ImColor(176, 44, 44, 255)));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(ImColor(209, 79, 79, 255)));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(ImColor(245, 103, 103, 255)));

		// Button style
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20.0f);

		if (ImGui::Button(" ", ImVec2(10.f, 10.f)))
			PostQuitMessage(0);

		ImGui::PopStyleVar(1);
		ImGui::PopStyleColor(3);
	}

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(ImColor(15, 15, 16, 255)));

	static std::vector<int> passwordLengths;
	static std::vector<std::string> generatedPasswords;
	static std::vector<std::string> passwordNames;

	// Password Box
	static char inputTextBuffer[256] = "";
	{
		ImGui::SetCursorPos({ 9.f, 40.f });
		ImGui::TextColored(ImVec4(ImColor(230, 230, 230, 255)), "enter name");

		// Input box color
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(ImColor(21, 21, 23, 255)));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(ImColor(255, 255, 255, 10)));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(ImColor(200, 200, 200, 255)));

		// Input box border
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.f);

		ImGui::PushItemWidth(170.f);
		ImGui::InputText("##passwordName", inputTextBuffer, IM_ARRAYSIZE(inputTextBuffer));

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(3);
	}

	// Length Box
	static int passwordLength = 32;
	{
		// Slider color
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(ImColor(21, 21, 23, 255)));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(ImColor(21, 21, 23, 255)));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(ImColor(21, 21, 23, 255)));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(ImColor(255, 255, 255, 10)));
		ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(ImColor(162, 102, 169, 255)));
		ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(ImColor(213, 125, 170, 255)));

		// Slider border
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.f);

		// Button colors
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(ImColor(0, 0, 0, 0)));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(ImColor(0, 0, 0, 0)));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(ImColor(0, 0, 0, 0)));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(ImColor(0, 0, 0, 0)));

		ImGui::SetCursorPos({ 87.f,85.f });
		ImGui::Text("%d", passwordLength);

		ImGui::SetCursorPosY(100.f);

		// "-" Button
		if (ImGui::Button("-", ImVec2(20, 20)))
		{
			if (passwordLength > 1)
				passwordLength--;
		}

		ImGui::SameLine();
		ImGui::SetNextItemWidth(110.f);

		// Slider
		ImGuiStyle& style = ImGui::GetStyle();
		style.FrameBorderSize = 1.0f;

		// Slider
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.26f);
		if (ImGui::SliderInt("", &passwordLength, 1, 64, ""))
		{
			if (passwordLength > 64)
				passwordLength = 64;
			else if (passwordLength < 1)
				passwordLength = 1;

			if (passwordNames.size() > 0)
			{
				int lastIndex = passwordNames.size() - 1;
				int lastPasswordLength = passwordLengths[lastIndex];
				if (passwordLength != lastPasswordLength)
					passwordLengths[lastIndex] = passwordLength;
			}
		}
		ImGui::PopItemWidth();

		// For Tooltip
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 20.f);

		// Tooltip color
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(ImColor(46, 46, 46, 255)));
		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(ImColor(27, 27, 28, 255)));

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Password length");

		ImGui::PopStyleVar(1);
		ImGui::PopStyleColor(2);
		ImGui::SameLine();

		// "+" Button
		if (ImGui::Button("+", ImVec2(20, 20)))
		{
			if (passwordLength < 64)
				passwordLength++;
		}

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(10);
	}

	// Settings list
	static bool allowLowerCase{ false };
	static bool allowUpperCase{ false };
	static bool allowDigits{ false };
	static bool allowSpecialCharacters{ false };
	static bool allowSymbols{ false };
	{
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(ImColor(25, 25, 26, 255)));

		ImGui::SetCursorPos({ 250.f, 40.f });
		ImGui::Text("settings");

		ImGui::SetCursorPos({ 250.f, 57.f });
		ImGui::Checkbox("abc", &allowLowerCase);

		ImGui::SetCursorPos({ 320.f, 57.f });
		ImGui::Checkbox("ABC", &allowUpperCase);

		ImGui::SetCursorPos({ 390.f, 57.f });
		ImGui::Checkbox("123", &allowDigits);

		ImGui::SetCursorPos({ 250.f, 90.f });
		ImGui::Checkbox("!@#", &allowSpecialCharacters);

		ImGui::PopStyleColor(1);
	}

	// Generate Button
	{
		// Check if input box is empty
		bool isInputEmpty = (strlen(inputTextBuffer) == 0);

		// Button color
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(ImColor(21, 21, 23, 255)));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(ImColor(39, 39, 43, 255)));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(ImColor(46, 46, 55, 255)));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(ImColor(255, 255, 255, 10)));

		// Button border
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.f);

		ImGui::SetCursorPos({ 150.f, 150.f });

		// If input box is not empty
		if ((!isInputEmpty && allowLowerCase) || (!isInputEmpty && allowUpperCase) || (!isInputEmpty && allowDigits) || (!isInputEmpty && allowSpecialCharacters))
		{
			if (ImGui::Button("generate", { 170.f, 20.f }))
			{
				auto it = std::find(passwordNames.begin(), passwordNames.end(), inputTextBuffer);
				if (it == passwordNames.end())
				{
					passwordNames.push_back(inputTextBuffer);
					passwordLengths.push_back(passwordLength);

					std::string generatedPassword;

					std::string charset;
					if (allowLowerCase) charset += "abcdefghijklmnopqrstuvwxyz";
					if (allowUpperCase) charset += "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
					if (allowDigits) charset += "0123456789";
					if (allowSpecialCharacters) charset += "!@#$%^&*()_+-=[]{}|;:,.<>?";

					if (charset.empty()) charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

					generatedPassword = GenerateRandomPassword(passwordLength, charset);

					ImGui::Text("%s: %s", inputTextBuffer, generatedPassword.c_str());
					generatedPasswords.push_back(generatedPassword);

					std::memset(inputTextBuffer, 0, sizeof(inputTextBuffer));
				}
			}
		}
		else
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(ImColor(21, 21, 23, 255)));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(ImColor(60, 21, 23, 255)));
			ImGui::Button("generate", { 170.f, 20.f });
			ImGui::PopStyleVar();
		}

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(4);
	}

	// Console Label
	{
		ImGui::SetCursorPos({ 10.f, 165.f });
		ImGui::TextColored(ImVec4(ImColor(192, 192, 193, 255)), "Console");
	}

	// Console Box [CHILD]
	{
		// Box color
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(ImColor(13, 14, 15, 255)));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(ImColor(29, 29, 29, 255)));

		// Scrollbar color
		ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(ImColor(13, 14, 15, 255)));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(ImColor(157, 100, 138, 255)));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(ImColor(181, 114, 159, 255)));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(ImColor(196, 124, 172, 255)));

		ImGui::SetCursorPosX(5.f);
		ImGui::BeginChild("ConsoleBackground", { 490.f, 105.f }, true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);

		// Buttons rounding
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 20.0);

		std::vector<bool> deleteButtonClicked(passwordNames.size(), false);
		for (size_t i = 0; i < passwordNames.size(); ++i)
		{
			logicButtons(i, generatedPasswords, passwordNames, deleteButtonClicked); // Delete, Copy button
			ImGui::SameLine();

			float lineHeight = ImGui::GetTextLineHeight();
			float buttonPosY = ImGui::GetCursorPosY();

			ImGui::SetCursorPosY(buttonPosY - lineHeight / 3);
			ImGui::Text(" %s:", passwordNames[i].c_str());

			ImGui::SameLine();
			ImGui::SetCursorPosY(buttonPosY - lineHeight / 3);
			ImGui::TextColored(ImVec4(ImColor(255, 92, 255, 255)), "%s", generatedPasswords[i].c_str());
		}

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(5);
		ImGui::EndChild();
	}

	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
	ImGui::End();
}
