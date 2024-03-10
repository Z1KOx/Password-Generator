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

	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Delete");

	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(ImColor(32, 32, 117, 255)));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(ImColor(50, 50, 191, 255)));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(ImColor(77, 77, 247, 255)));
	if (ImGui::Button(("##Copy" + std::to_string(index)).c_str(), buttonSize))
	{
		ImGui::SetClipboardText(generatedPassword.c_str());
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Copy");

	ImGui::PopStyleColor();
}

void gui::Render() noexcept
{
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ WIDTH, HEIGHT });
	ImGui::Begin(
		"Password Generator",
		&isRunning,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove
	);

	static std::vector<std::string> passwordNames;
	static std::vector<int> passwordLengths;
	static std::vector<std::string> generatedPasswords;

	// Password Box
	static char inputTextBuffer[256] = "";
	{
		ImGui::SetCursorPos({ 9.f ,30.f });
		ImGui::Text("enter name");
		ImGui::PushItemWidth(170.f);
		ImGui::InputText("##passwordName", inputTextBuffer, IM_ARRAYSIZE(inputTextBuffer));
	}

	// Length Box
	static int passwordLength = 32;
	{
		ImGui::SetCursorPosY(75.f);
		if (ImGui::SliderInt(" ", &passwordLength, 1, 64))
		{
			if (passwordNames.size() > 0)
			{
				int lastIndex = passwordNames.size() - 1;
				int lastPasswordLength = passwordLengths[lastIndex];
				if (passwordLength != lastPasswordLength)
				{
					passwordLengths[lastIndex] = passwordLength;
				}
			}
		}

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Password length");
	}

	// Generate Button
	{
		ImGui::SetCursorPosY(120.f);
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
				font->FontSize = 15.f;
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(ImColor(255, 92, 255, 255)), "%s", generatedPassword.c_str());
				font->FontSize = defaultFontSize;

				std::memset(inputTextBuffer, 0, sizeof(inputTextBuffer));
			}
		}
	}

	// Passwords Label
	{
		ImGui::SetCursorPos({9.f, 170.f });
		ImGui::Text("passwords");
	}

	// Passwords Box [CHILD]
	{
		ImGui::SetCursorPosX(5.f);
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(ImColor(255, 255, 255, 10)));
		ImGui::BeginChild("PasswordsBackground", { 490.f, 105.f }, true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);

		for (size_t i = 0; i < passwordNames.size(); ++i)
		{
			logicButtons(i, generatedPasswords[i]); // Delete, Copy button
			ImGui::SameLine();

			float lineHeight = ImGui::GetTextLineHeight();
			float buttonPosY = ImGui::GetCursorPosY();

			ImGui::SetCursorPosY(buttonPosY - lineHeight / 3);
			ImGui::Text("%s:", passwordNames[i].c_str());

			ImGui::SameLine();
			ImGui::SetCursorPosY(buttonPosY - lineHeight / 3);
			ImGui::TextColored(ImVec4(ImColor(255, 92, 255, 255)), "%s", generatedPasswords[i].c_str());
		}

		ImGui::EndChild();
	}

	ImGui::End();
}
