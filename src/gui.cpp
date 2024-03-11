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

void logicButtons(int index, const std::string& generatedPassword)
{
	ImVec2 buttonSize(10.f, 10.f);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(ImColor(117, 32, 32, 255)));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(ImColor(191, 50, 50, 255)));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(ImColor(247, 77, 77, 255)));

	if (ImGui::Button(("##Delete" + std::to_string(index)).c_str(), buttonSize))
	{
		// Placeholder for delete logic
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Delete");

	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(ImColor(32, 32, 117, 255)));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(ImColor(50, 50, 191, 255)));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(ImColor(77, 77, 247, 255)));

	if (ImGui::Button(("##Copy" + std::to_string(index)).c_str(), buttonSize))
		ImGui::SetClipboardText(generatedPassword.c_str());
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Copy");

	ImGui::PopStyleColor(6);
}

void gui::Render() noexcept
{
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ WIDTH, HEIGHT });

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	ImGui::Begin(
		"Password Generator 0.0.0.4",
		&isRunning,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoDecoration
	);

	ImGui::SetCursorPos({ 0.f, 5.f });

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(ImColor(28, 28, 29, 255)));
	ImGui::BeginChild("TitleBar", ImVec2(495.f, 20.f));
	{
		ImGui::SetCursorPos({ 11.f, 3.f });
		ImGui::Text("Password Generator                                         0.0.0.6");
		ImGui::PopStyleColor(1);

		
		ImGui::SetCursorPos(ImVec2(0.f, 19.f));

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(ImColor(136, 86, 119, 255)));
		ImGui::BeginChild(" ", ImVec2(495.f, 1.f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs);
		{
			ImGui::PopStyleColor(1);
			ImGui::EndChild();
		}
		
		ImGui::EndChild();
	}

	ImGui::PopStyleVar(1);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(ImColor(15, 15, 16, 255)));
	
	static std::vector<std::string> passwordNames;
	static std::vector<int> passwordLengths;
	static std::vector<std::string> generatedPasswords;

	// Password Box
	static char inputTextBuffer[256] = "";
	{
		ImGui::SetCursorPos({ 9.f ,40.f });
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
	static int passwordLength = 14;
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

		ImGui::SetCursorPos({ 83.f,85.f });
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
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.26f);
		if (ImGui::SliderInt("", &passwordLength, 1, 64, "")) 
		{
			// Checking if slider goes out of range
			if (passwordLength > 64)
				passwordLength = 64;
			else if (passwordLength < 1)
				passwordLength = 1;

			if (passwordNames.size() > 0) {
				int lastIndex = passwordNames.size() - 1;
				int lastPasswordLength = passwordLengths[lastIndex];
				if (passwordLength != lastPasswordLength)
					passwordLengths[lastIndex] = passwordLength;
			}
		}
		ImGui::PopItemWidth();


		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Password length");

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

		ImGui::SetCursorPos({ 150.f, 160.f });

		// If input box is not empty
		if (!isInputEmpty)
		{
			if (ImGui::Button("generate", { 170.f, 20.f }))
			{
				auto it = std::find(passwordNames.begin(), passwordNames.end(), inputTextBuffer);
				if (it == passwordNames.end())
				{
					passwordNames.push_back(inputTextBuffer);
					passwordLengths.push_back(passwordLength);

					std::string generatedPassword = GenerateRandomPassword(passwordLength);
					generatedPasswords.push_back(generatedPassword);

					ImFont* font = ImGui::GetFont();
					float defaultFontSize = font->FontSize;

					if (passwordNames.size() == 1)
						ImGui::SetCursorPos({ 0.f, 200.f });
					else
						ImGui::SetCursorPos({ 0.f, 200.f + static_cast<float>(passwordNames.size() - 1) * 20.f });

					int numLines = passwordLength / 2;
					for (int i = 0; i < numLines; ++i)
						ImGui::Text(" ");

					ImGui::Text("%s:", inputTextBuffer);
					font->FontSize = 15.f; // Make the font size smaller for generatedPassword
					ImGui::SameLine();
					ImGui::TextColored(ImVec4(255.0f / 255.0f, 92.0f / 255.0f, 255.0f / 255.0f, 1.0f), "%s", generatedPassword.c_str());
					font->FontSize = defaultFontSize;

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
		ImGui::SetCursorPos({ 5.f, 160.f });

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(ImColor(13, 14, 15, 255)));
		ImGui::BeginChild("Console label background", { 50.f, 20.f });
		{
			ImGui::SetCursorPosY(5.f);
			ImGui::TextColored(ImVec4(ImColor(230, 230, 230, 255)), "Console");
			ImGui::PopStyleColor(1);
			ImGui::EndChild();
		}
	}

	// Console Box [CHILD]
	{
		// Box color
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(ImColor(13, 14, 15, 255)));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(ImColor(14, 14, 15, 255)));

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

		for (size_t i = 0; i < passwordNames.size(); ++i)
		{
			logicButtons(i, generatedPasswords[i]); // Delete, Copy button
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
