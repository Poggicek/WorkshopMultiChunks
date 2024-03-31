/**
 * =============================================================================
 * CS2Fixes
 * Copyright (C) 2023 Source2ZE
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include "plat.h"
#include <filesystem>
#include <Windows.h>
#include <cstdint>

#ifdef _WIN32
#include <Psapi.h>
#endif

enum SigError
{
	SIG_OK,
	SIG_NOT_FOUND,
	SIG_FOUND_MULTIPLE,
};

void Error(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), fmt, args);

	va_end(args);

	MessageBoxA(nullptr, buffer, "Error", MB_OK | MB_ICONERROR);
	exit(1);
}

void Info(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), fmt, args);

	va_end(args);

	MessageBoxA(nullptr, buffer, "Info", MB_OK | MB_ICONINFORMATION);
}

class CModule
{
public:
	CModule(const char *path, const char *module) :
		m_pszModule(module), m_pszPath(path)
	{
		char szModule[MAX_PATH];
		auto binPath = std::filesystem::current_path().parent_path().parent_path();

		snprintf(szModule, MAX_PATH, "%ls%s%s%s%s", binPath.c_str(), path, MODULE_PREFIX, m_pszModule, MODULE_EXT);

		m_hModule = LoadLibraryA(szModule);

		if (!m_hModule)
			Error("Could not find %s\n", szModule);

#ifdef _WIN32
		MODULEINFO m_hModuleInfo;
		GetModuleInformation(GetCurrentProcess(), m_hModule, &m_hModuleInfo, sizeof(m_hModuleInfo));

		m_base = (void *)m_hModuleInfo.lpBaseOfDll;
		m_size = m_hModuleInfo.SizeOfImage;
#else
		if (int e = GetModuleInformation(m_hModule, &m_base, &m_size))
			Error("Failed to get module info for %s, error %d\n", szModule, e);
#endif
	}


	void *FindSignature(const uint8_t *pData, size_t iSigLength, int &error)
	{
		unsigned char *pMemory;
		void *return_addr = nullptr;
		error = 0;

		pMemory = (uint8_t*)m_base;

		for (size_t i = 0; i < m_size; i++)
		{
			size_t Matches = 0;
			while (*(pMemory + i + Matches) == pData[Matches] || pData[Matches] == '\x2A')
			{
				Matches++;
				if (Matches == iSigLength)
				{
					if (return_addr)
					{
						error = SIG_FOUND_MULTIPLE;
						return return_addr;
					}

					return_addr = (void *)(pMemory + i);
				}
			}
		}

		if (!return_addr)
			error = SIG_NOT_FOUND;

		return return_addr;
	}

	const char *m_pszModule;
	const char* m_pszPath;
	HINSTANCE m_hModule;
	void* m_base;
	size_t m_size;
};