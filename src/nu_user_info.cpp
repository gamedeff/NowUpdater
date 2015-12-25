//
// NowUpdater
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#include "nu_user_info.h"

#include "Poco/StringTokenizer.h"

#include "Poco/File.h"

#include "Poco/NumberFormatter.h"

#include "Poco/DateTimeFormat.h"
#include "Poco/DateTimeFormatter.h"

#include "closure.h"

#include "imgui.h"

#if defined(_DEBUG)
#define IMGUI_LIB_SUFFIX "_d.lib"
#else
#define IMGUI_LIB_SUFFIX ".lib"
#endif

#pragma comment(lib, "imgui" IMGUI_LIB_SUFFIX)

bool ImplDX9_LoadImage(const void *data, uint32_t data_size, title_image_t &image);

#if 0

#include <windows.h>

typedef long (NTAPI *_NtQueryInformationProcess)(
	HANDLE ProcessHandle,
	DWORD ProcessInformationClass,
	PVOID ProcessInformation,
	DWORD ProcessInformationLength,
	PDWORD ReturnLength
	);

typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _PROCESS_BASIC_INFORMATION
{
	LONG ExitStatus;
	PVOID PebBaseAddress;
	ULONG_PTR AffinityMask;
	LONG BasePriority;
	ULONG_PTR UniqueProcessId;
	ULONG_PTR ParentProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

PVOID GetPebAddress(HANDLE ProcessHandle)
{
	_NtQueryInformationProcess NtQueryInformationProcess =
		(_NtQueryInformationProcess)GetProcAddress(
		GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");
	PROCESS_BASIC_INFORMATION pbi;

	NtQueryInformationProcess(ProcessHandle, 0, &pbi, sizeof(pbi), NULL);

	return pbi.PebBaseAddress;
}

int GetProcessCommandLine(int pid)
{
	HANDLE processHandle;
	PVOID pebAddress;
	PVOID rtlUserProcParamsAddress;
	UNICODE_STRING commandLine;
	WCHAR *commandLineContents;

	if ((processHandle = OpenProcess(
		PROCESS_QUERY_INFORMATION | /* required for NtQueryInformationProcess */
		PROCESS_VM_READ, /* required for ReadProcessMemory */
		FALSE, pid)) == 0)
	{
		printf("Could not open process!\n");
		return GetLastError();
	}

	pebAddress = GetPebAddress(processHandle);

	/* get the address of ProcessParameters */
	if (!ReadProcessMemory(processHandle, (PCHAR)pebAddress + 0x10,
		&rtlUserProcParamsAddress, sizeof(PVOID), NULL))
	{
		printf("Could not read the address of ProcessParameters!\n");
		return GetLastError();
	}

	/* read the CommandLine UNICODE_STRING structure */
	if (!ReadProcessMemory(processHandle, (PCHAR)rtlUserProcParamsAddress + 0x40,
		&commandLine, sizeof(commandLine), NULL))
	{
		printf("Could not read CommandLine!\n");
		return GetLastError();
	}

	/* allocate memory to hold the command line */
	commandLineContents = (WCHAR *)malloc(commandLine.Length);

	/* read the command line */
	if (!ReadProcessMemory(processHandle, commandLine.Buffer,
		commandLineContents, commandLine.Length, NULL))
	{
		printf("Could not read the command line string!\n");
		return GetLastError();
	}

	/* print it */
	/* the length specifier is in characters, but commandLine.Length is in bytes */
	/* a WCHAR is 2 bytes */
	printf("%.*S\n", commandLine.Length / 2, commandLineContents);
	CloseHandle(processHandle);
	free(commandLineContents);

	return 1;
}

#include <tchar.h>
#include <tlhelp32.h>

//  Forward declarations:
BOOL GetProcessList();
BOOL ListProcessModules( DWORD dwPID );
BOOL ListProcessThreads( DWORD dwOwnerPID );
void printError( TCHAR* msg );

BOOL GetProcessList()
{
	HANDLE hProcessSnap;
	HANDLE hProcess;
	PROCESSENTRY32 pe32;
	DWORD dwPriorityClass;

	// Take a snapshot of all processes in the system.
	hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if( hProcessSnap == INVALID_HANDLE_VALUE )
	{
		printError( TEXT("CreateToolhelp32Snapshot (of processes)") );
		return( FALSE );
	}

	// Set the size of the structure before using it.
	pe32.dwSize = sizeof( PROCESSENTRY32 );

	// Retrieve information about the first process,
	// and exit if unsuccessful
	if( !Process32First( hProcessSnap, &pe32 ) )
	{
		printError( TEXT("Process32First") ); // show cause of failure
		CloseHandle( hProcessSnap );          // clean the snapshot object
		return( FALSE );
	}

	// Now walk the snapshot of processes, and
	// display information about each process in turn
	do
	{
		_tprintf( TEXT("\n\n=====================================================" ));
		_tprintf( TEXT("\nPROCESS NAME:  %s"), pe32.szExeFile );
		_tprintf( TEXT("\n-------------------------------------------------------" ));

		// Retrieve the priority class.
		dwPriorityClass = 0;
		hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID );
		if( hProcess == NULL )
			printError( TEXT("OpenProcess") );
		else
		{
			dwPriorityClass = GetPriorityClass( hProcess );
			if( !dwPriorityClass )
				printError( TEXT("GetPriorityClass") );
			CloseHandle( hProcess );
		}

		_tprintf( TEXT("\n  Process ID        = 0x%08X"), pe32.th32ProcessID );
		_tprintf( TEXT("\n  Thread count      = %d"),   pe32.cntThreads );
		_tprintf( TEXT("\n  Parent process ID = 0x%08X"), pe32.th32ParentProcessID );
		_tprintf( TEXT("\n  Priority base     = %d"), pe32.pcPriClassBase );
		if( dwPriorityClass )
			_tprintf( TEXT("\n  Priority class    = %d"), dwPriorityClass );

		// List the modules and threads associated with this process
		ListProcessModules( pe32.th32ProcessID );
		//ListProcessThreads( pe32.th32ProcessID );
		GetProcessCommandLine(pe32.th32ProcessID);

	} while( Process32Next( hProcessSnap, &pe32 ) );

	CloseHandle( hProcessSnap );
	return( TRUE );
}


BOOL ListProcessModules( DWORD dwPID )
{
	HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
	MODULEENTRY32 me32;

	// Take a snapshot of all modules in the specified process.
	hModuleSnap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, dwPID );
	if( hModuleSnap == INVALID_HANDLE_VALUE )
	{
		printError( TEXT("CreateToolhelp32Snapshot (of modules)") );
		return( FALSE );
	}

	// Set the size of the structure before using it.
	me32.dwSize = sizeof( MODULEENTRY32 );

	// Retrieve information about the first module,
	// and exit if unsuccessful
	if( !Module32First( hModuleSnap, &me32 ) )
	{
		printError( TEXT("Module32First") );  // show cause of failure
		CloseHandle( hModuleSnap );           // clean the snapshot object
		return( FALSE );
	}

	// Now walk the module list of the process,
	// and display information about each module
	do
	{
		_tprintf( TEXT("\n\n     MODULE NAME:     %s"),   me32.szModule );
		_tprintf( TEXT("\n     Executable     = %s"),     me32.szExePath );
		_tprintf( TEXT("\n     Process ID     = 0x%08X"),         me32.th32ProcessID );
		_tprintf( TEXT("\n     Ref count (g)  = 0x%04X"),     me32.GlblcntUsage );
		_tprintf( TEXT("\n     Ref count (p)  = 0x%04X"),     me32.ProccntUsage );
		_tprintf( TEXT("\n     Base address   = 0x%08X"), (DWORD) me32.modBaseAddr );
		_tprintf( TEXT("\n     Base size      = %d"),             me32.modBaseSize );

	} while( 0 && Module32Next( hModuleSnap, &me32 ) );

	CloseHandle( hModuleSnap );
	return( TRUE );
}

BOOL ListProcessThreads( DWORD dwOwnerPID ) 
{ 
	HANDLE hThreadSnap = INVALID_HANDLE_VALUE; 
	THREADENTRY32 te32; 

	// Take a snapshot of all running threads  
	hThreadSnap = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 ); 
	if( hThreadSnap == INVALID_HANDLE_VALUE ) 
		return( FALSE ); 

	// Fill in the size of the structure before using it. 
	te32.dwSize = sizeof(THREADENTRY32); 

	// Retrieve information about the first thread,
	// and exit if unsuccessful
	if( !Thread32First( hThreadSnap, &te32 ) ) 
	{
		printError( TEXT("Thread32First") ); // show cause of failure
		CloseHandle( hThreadSnap );          // clean the snapshot object
		return( FALSE );
	}

	// Now walk the thread list of the system,
	// and display information about each thread
	// associated with the specified process
	do 
	{ 
		if( te32.th32OwnerProcessID == dwOwnerPID )
		{
			_tprintf( TEXT("\n\n     THREAD ID      = 0x%08X"), te32.th32ThreadID ); 
			_tprintf( TEXT("\n     Base priority  = %d"), te32.tpBasePri ); 
			_tprintf( TEXT("\n     Delta priority = %d"), te32.tpDeltaPri ); 
			_tprintf( TEXT("\n"));
		}
	} while( Thread32Next(hThreadSnap, &te32 ) ); 

	CloseHandle( hThreadSnap );
	return( TRUE );
}

void printError( TCHAR* msg )
{
	DWORD eNum;
	TCHAR sysMsg[256];
	TCHAR* p;

	eNum = GetLastError( );
	FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, eNum,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		sysMsg, 256, NULL );

	// Trim the end of the line and terminate it with a null
	p = sysMsg;
	while( ( *p > 31 ) || ( *p == 9 ) )
		++p;
	do { *p-- = 0; } while( ( p >= sysMsg ) &&
		( ( *p == '.' ) || ( *p < 33 ) ) );

	// Display the message
	_tprintf( TEXT("\n  WARNING: %s failed with error %d (%s)"), msg, eNum, sysMsg );
}

#endif

std::string get_data_path(options_t *options, const std::string &dataname, bool createdirs = false)
{
	assert(options);

	std::string datapath = Path::dataHome() + options->app_name + Path::separator() + options->data_dir;

	if(createdirs)
		Poco::File(datapath).createDirectories();

	return datapath + Path::separator() + dataname + options->xml_ext;
}

bool load_data(options_t *options, const std::string &dataname, const pugi::char_t *dataset, Closure<bool(pugi::xml_node &)> read_func)
{
	std::string filename = get_data_path(options, dataname);

	pugi::xml_document doc;

	pugi::xml_parse_result result = doc.load_file(filename.c_str());

	if(!check_xml_parse_result(result, " at " + filename))
		return false;

	if(!read_func(doc.child(dataset)))
		return false;

	return true;
}

bool save_data(options_t *options, const std::string &dataname, const pugi::char_t *dataset, Closure<bool(pugi::xml_node &)> write_func)
{
	std::string filename = get_data_path(options, dataname, true);

	pugi::xml_document doc;

	if(!write_func(doc.append_child(dataset)))
		return false;

	if(!doc.save_file(filename.c_str()))
		return false;

	return true;
}


user_info_t::user_info_t(const std::string &username, const std::string &password, options_t *options) : username(username), password(password), options(options), current_site(0)//, poco("myanimelist.net")//poco("www.anime-planet.com") //poco(sites[current_site].site_url)
{
	sites.push_back(new anime_planet_site_info_t());

	sites.push_back(new myanimelist_site_info_t());

	sites.push_back(new imdb_site_info_t());

//#define NU_FUNC_IMDB(x) NU_FUNC(x, imdb)
//
//	NU_FUNC_IMDB(authenticate);
//	NU_FUNC_IMDB(parse_title_info);
//	NU_FUNC_IMDB(sync);
//	//NU_FUNC_IMDB(send_request_change_title_episodes_watched_num);
//	NU_FUNC_IMDB(send_request_change_title_status);
//	NU_FUNC_IMDB(send_request_change_title_rating);
//	NU_FUNC_IMDB(send_request_add_title);
//	NU_FUNC_IMDB(send_request_delete_title);
//	NU_FUNC_IMDB(send_request_search_title);
//
//#undef NU_FUNC_IMDB
//
//#undef NU_FUNC

	mediaplayers.push_back(nu_mediaplayer(_T("mpc-hc.exe")));

	show_title_popup = false;

	current_title_index = 0;
}

user_info_t::~user_info_t()
{
	//for(std::vector<site_info_t>::iterator it = sites.begin(); it != sites.end(); ++it)
	//{
	//	delete it->http;
	//}
}

void user_info_t::on_timer(Poco::Timer& timer)
{
	Poco::FastMutex::ScopedLock lock(mutex);

	//GetProcessList();
	for(std::vector<nu_mediaplayer>::const_iterator it = mediaplayers.begin(); it != mediaplayers.end(); ++it)
	{
		string_t title_filename = it->get_title_filename();
		tcout << title_filename << std::endl;

		if(!title_filename.empty())
		{
			title_filename_info_t title_filename_info = it->get_title_filename_info(title_filename);
			tcout << title_filename_info.title << " #" << title_filename_info.episode_number << std::endl;

			if(last_title.title != title_filename_info.title || last_title.episode_number != title_filename_info.episode_number)
			{
				uint32_t (user_info_t::*get_user_title_index_procs[])(uint32_t si, const std::string &title_name) =
				{
					&user_info_t::get_user_title_index,		  // find in user's titlelist
					&user_info_t::get_user_title_index_parse, // find in site's titlelist
					&user_info_t::find_and_add_title
				};

				//bool title_found = false;
				uint32_t title_index = GW_NOT_FOUND;

				for(uint32_t si = 0; si < site_users.size(); ++si)
					if(site_users[si].enabled)
					{
						for(uint32_t i = 0; i < countof(get_user_title_index_procs); ++i)
						{
							title_index = (this->*get_user_title_index_procs[i])(si, GW_T2A(title_filename_info.title.c_str()));

							if(title_index != GW_NOT_FOUND)
							{
								current_site = si;
								current_title_index = title_index;
								set_title_episodes_watched_num(si, current_title_index, title_filename_info.episode_number);
								show_title_popup = true;
								break;
								//title_found = true;
							}
						}

						//if(title_found)
						//	break;
					}

				last_title = title_filename_info;
			}
			else if(show_title_popup)
				show_title_popup = false;
		}
		else if(show_title_popup)
			show_title_popup = false;

		/*std::wstring cmd = GetProcessCommandLine(it->c_str());
		std::wcout << cmd << std::endl;

		const std::wstring space = L" ";
		std::wstring::size_type space_pos = cmd.find(space);
		if(space_pos != std::wstring::npos)
		{
			const std::wstring quote = L"\"";
			std::wstring::size_type start = 0;
			std::wstring::size_type quote_2nd_pos = std::wstring::npos;
			std::wstring::size_type quote_3rd_pos = std::wstring::npos;
			std::wstring::size_type quote_4th_pos = std::wstring::npos;
			uint32_t occurrences = 0;
			while((start = cmd.find(quote, start)) != std::wstring::npos)
			{
				++occurrences;
				if(occurrences == 2)
					quote_2nd_pos = start;
				else if(occurrences == 3)
					quote_3rd_pos = start;
				else if(occurrences == 4)
				{
					quote_4th_pos = start;
					break;
				}
				start += quote.length();
			}
			std::wstring token;
			if(occurrences >= 4 && space_pos > quote_2nd_pos && space_pos < quote_3rd_pos)
				token = cmd.substr(quote_3rd_pos + quote.length(), quote_4th_pos - quote_3rd_pos - quote.length());
			else if(occurrences >= 2 && space_pos > quote_2nd_pos)
				token = cmd.substr(space_pos + space.length());
			std::string utf8token;
			Poco::UnicodeConverter::toUTF8(token, utf8token);
			Poco::Path path(utf8token);
			std::string utf8filename = path.getFileName();
			std::wstring filename;
			Poco::UnicodeConverter::toUTF16(utf8filename, filename);
			std::wcout << filename << std::endl;
			anitomy::Anitomy anitomy;
			anitomy.Parse(filename);
			anitomy::Elements& elements = anitomy.elements();
			std::wcout << elements.get(anitomy::kElementAnimeTitle) << L" #" <<
						elements.get(anitomy::kElementEpisodeNumber) << L" by " <<
						elements.get(anitomy::kElementReleaseGroup) << std::endl;
		}*/
		//Poco::StringTokenizer st((*it), " ", Poco::StringTokenizer::TOK_TRIM | Poco::StringTokenizer::TOK_IGNORE_EMPTY);
		//for(Poco::StringTokenizer::Iterator s_it = st.begin(); s_it != st.end(); ++s_it)
		//{
		//	Poco::Path path((*s_it));
		//	std::cout << path.getFileName() << std::endl;
		//}
	}
}

bool user_info_t::load()
{
	if(!load_data(options, username, "user", CLOSURE(this, &user_info_t::read)))
		;//return false;

	for(uint32_t i = 0; i < site_users.size(); ++i)
		if(!load_data(options, sites[site_users[i].site_index]->name, "site", CLOSURE(sites[site_users[i].site_index], &site_info_t::read)))
			return false;

	if(site_users.empty())
		for(uint32_t i = 0; i < sites.size(); ++i)
		{
			site_user_info_t sui = { i, true, username, password };

			site_users.push_back(sui);
		}

	return true;
}

bool user_info_t::save()
{
	for(uint32_t i = 0; i < site_users.size(); ++i)
		if(!save_data(options, sites[site_users[i].site_index]->name, "site", CLOSURE(sites[site_users[i].site_index], &site_info_t::write)))
			return false;

	if(!save_data(options, username, "user", CLOSURE(this, &user_info_t::write)))
		return false;

	return true;
}

bool user_info_t::init()
{
	//std::string site_filename = Path::dataHome() + options->data_dir + sites[current_site].name + options->xml_ext;
	//std::string user_filename = Path::dataHome() + options->data_dir + username + options->xml_ext;

	if(!load())
		;

	if(!site_users.back().login_cookies.empty())
		for(std::map<std::string, std::string>::const_iterator login_cookies_it = site_users.back().login_cookies.begin(); login_cookies_it != site_users.back().login_cookies.end(); ++login_cookies_it)
			sites.back()->http->combined_cookies.push_back(HTTPCookie(login_cookies_it->first, login_cookies_it->second));
	else
		if(!sites.back()->authenticate(site_users.back()))
			return false;
		else
			for(std::vector<HTTPCookie>::const_iterator combined_cookies_it = sites.back()->http->combined_cookies.begin(); combined_cookies_it != sites.back()->http->combined_cookies.end(); ++combined_cookies_it)
				site_users.back().login_cookies[combined_cookies_it->getName()] = combined_cookies_it->getValue();

	title_info_t title;
	title.name = "Pulp Fiction";
	title.id = 110912;
	//sites.back().send_request_change_title_rating(site_users.back(), title, 4);
	//sites.back().send_request_add_title_imdb(site_users.back(), title, NU_TITLE_STATUS_PLAN_TO_WATCH);
	//sites.back().send_request_delete_title_imdb(site_users.back(), title);

	//pugi::xml_document doc;

	//std::string imdb_title_id = sites.back().imdb_get_title_id(title.name);
	//if(!load_xhtml(doc, sites.back().http->redirect_to("http://www.imdb.com/title/" + imdb_title_id + "/", sites.back().login_cookie)))
	//	return false;
	//site_user_info_t su;
	//sites.back().parse_title_info_imdb(doc, su, title);

	////sites.back().get_tconst("Pulp Fiction");
	////sites.back().imdb_get_auth_token("tt0110912");
	////std::string login_cookie;
	//if(!sites.back().login_cookie.empty())
	//{
	//	char login_cookie_delim = '|';
	//	std::vector<std::string> cookie_names = split(sites.back().login_cookie, login_cookie_delim);
	//	for(int i = 0; i < cookie_names.size(); ++i)
	//		for(std::vector<HTTPCookie>::const_iterator combined_cookies_it = sites.back().http->combined_cookies.begin(); combined_cookies_it != sites.back().http->combined_cookies.end(); ++combined_cookies_it)
	//			if(combined_cookies_it->getName() == cookie_names[i])
	//			{
	//				//login_cookie += (login_cookie.empty() ? "" : std::string() + login_cookie_delim) + combined_cookies_it->getValue();
	//				site_users.back().login_cookies[combined_cookies_it->getName()] = combined_cookies_it->getValue();
	//				break;
	//			}
	//}
	//site_users.back().login_cookie = login_cookie;
	//sites.back().sync_imdb(site_users.back().username, site_users.back().password, site_users.back());

	//std::string filename = get_data_path(options, "animelist_1430539159_-_124542");

	//pugi::xml_document doc;

	//pugi::xml_parse_result result = doc.load_file(filename.c_str());

	//if(!check_xml_parse_result(result, " at " + filename))
	//	return false;

	//sites.back().import_mal(doc, site_users.back());

	//sync_all();
	//if(!sync())
	//	return false;

	if(!save())
		return false;

	//{
	//	pugi::xml_document doc;

	//	if(!sites[current_site].write(doc.append_child("site")))
	//		return false;

	//	if(!doc.save_file(site_filename.c_str()))
	//		return false;
	//}

	//{
	//	pugi::xml_document doc;

	//	if(!write(doc.append_child("user")))
	//		return false;

	//	if(!doc.save_file(user_filename.c_str()))
	//		return false;
	//}

	return true;
}

bool user_info_t::set_title_episodes_watched_num(uint32_t si, uint32_t i, uint32_t episodes_watched_num)
{
	if(site_users[si].user_titles[i].status == NU_TITLE_STATUS_NOT_ADDED || site_users[si].user_titles[i].status == NU_TITLE_STATUS_PLAN_TO_WATCH)
		if(sites[site_users[si].site_index]->send_request_change_title_status(site_users[si], site_users[si].user_titles[i].index, NU_TITLE_STATUS_WATCHING))
			site_users[si].user_titles[i].status = NU_TITLE_STATUS_WATCHING;
		else
			return false;

	if(sites[site_users[si].site_index]->send_request_change_title_episodes_watched_num(site_users[si], site_users[si].user_titles[i].index, episodes_watched_num))
		site_users[si].user_titles[i].episodes_watched_num = episodes_watched_num;
	else
		return false;

	return true;
}

bool user_info_t::set_title_episodes_watched_num(uint32_t i, uint32_t episodes_watched_num)
{
	for(uint32_t si = 0; si < site_users.size(); ++si)
		if(site_users[si].enabled)
			if(si == current_site || has_title(si, current_site, i))
				if(!set_title_episodes_watched_num(si, i, episodes_watched_num))
					return false;

	return true;
}

bool user_info_t::add_title(uint32_t si, title_info_t &title, uint32_t status)
{
	//if(!title.uri.empty())
	{
		if(!sites[site_users[si].site_index]->parse_title_info(site_users[si], /*title.name, */title.uri, title))
			return false;
	}

	user_title_info_t user_title = { 0 };

	user_title.index = sites[site_users[si].site_index]->titles.size();

	//if(!title.uri.empty())
	{
		if(!sites[site_users[si].site_index]->parse_user_title_info(site_users[si], title, user_title))
			return false;
	}

	if(user_title.status != status)
	{
		if(!sites[site_users[si].site_index]->send_request_add_title(site_users[si], title, status))
			return false;

		user_title.status = status;
	}

	site_users[si].user_titles.push_back(user_title);

	sites[site_users[si].site_index]->titles.push_back(title);

	add_to_history(NU_ACT_ADD_TITLE, _FS(_T("\"%s\" on %s"), GW_A2T(title.name), GW_A2T(sites[site_users[si].site_index]->name)));

	return true;
}

bool user_info_t::add_title(title_info_t &title, uint32_t status)
{
	for(uint32_t si = 0; si < site_users.size(); ++si)
		if(site_users[si].enabled)
			if(!add_title(si, title, status))
				return false;

	return true;
}

bool user_info_t::search_title(uint32_t si, const std::string &title_name, std::vector<title_info_t> &found_titles)
{
	return sites[site_users[si].site_index]->send_request_search_title(site_users[si], title_name, found_titles);
}

bool user_info_t::search_title(const std::string &title_name, std::vector<title_info_t> &found_titles)
{
	for(uint32_t si = 0; si < site_users.size(); ++si)
		if(site_users[si].enabled)
		if(!search_title(si, title_name, found_titles))
			return false;

	return true;
}


const char *GetSortDirectionLabel(bool sortSelected, bool descending)
{
	if(!sortSelected)
		return "";

	if(descending)
		return "v";
	else
		return "^";
}

bool ImGuiSelectable(bool selected, int id, const char *format, ...)
{
	char buffer[1024] = {};
	va_list list;
	va_start(list, format);
	vsnprintf(buffer, sizeof(buffer), format, list);
	va_end(list);

	ImGui::PushID(id);
	bool changed = ImGui::Selectable(buffer, selected);
	ImGui::PopID();

	return changed;
}

struct ImGuiColumn
{
	float display_offset;
	std::string title;
	void* compare_func;
};

//static bool Items_TitleMapGetter(void* data, int idx, const char** out_text)
//{
//	std::map<std::string, user_title_info_t>::const_iterator *pitems_it = (std::map<std::string, user_title_info_t>::const_iterator *) data;
//	if(out_text)
//	{
//		std::map<std::string, user_title_info_t>::const_iterator items_it = (*pitems_it);
//		while(idx--)
//		{
//			assert(idx >= 0);
//			items_it++;
//		}
//		*out_text = items_it->first.c_str();
//	}
//	return true;
//}

static bool Items_TitleGetter(void* data, int idx, const char** out_text)
{
	static char search_str_fmt[64] = "%s (%d)";

	std::vector<title_info_t>::const_iterator *pitems_it = (std::vector<title_info_t>::const_iterator *) data;
	if(out_text)
	{
		std::vector<title_info_t>::const_iterator items_it = (*pitems_it) + idx;

		//*out_text = items_it->name.c_str();
		*out_text = _FS_narrow(search_str_fmt, items_it->name.c_str(), items_it->year);
	}
	return true;
}

static bool Items_TitleStylerBegin(void* data, int idx)
{
	user_info_t *user_info = (user_info_t *) data;

	bool already_added = false;
	for(std::vector<title_info_t>::const_iterator it = user_info->sites[0]->titles.begin(); it != user_info->sites[0]->titles.end(); ++it)
		if(it->name == user_info->last_found_titles[idx].name)
		{
			already_added = true;
			break;
		}

	ImVec4 already_added_color(1.0f, 1.0f, 0.0f, 0.0f), normal_color(1.0f, 1.0f, 1.0f, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Text, already_added ? already_added_color : normal_color);
	//if(already_added)
	//	ImGui::PushStyleColor(ImGuiCol_Text, already_added_color);

	return true;
}

static bool Items_TitleStylerEnd(void* data, int idx)
{
	//user_info_t *user_info = (user_info_t *) data;

	//bool already_added = false;
	//for(std::vector<title_info_t>::const_iterator it = user_info->sites[0].titles.begin(); it != user_info->sites[0].titles.end(); ++it)
	//	if(it->name == user_info->found_titles[idx].name)
	//	{
	//		already_added = true;
	//		break;
	//	}

	//if(already_added)
		ImGui::PopStyleColor();

	return true;
}

char IndexNumberArrayGetterBuffer[2 * _MAX_INT_DIG];

static bool Items_IndexNumberArrayGetter(void* data, int idx, const char** out_text)
{
	int start_idx = (int) data;

	sprintf_s(IndexNumberArrayGetterBuffer, sizeof(IndexNumberArrayGetterBuffer), "%d", start_idx + idx);

	if(out_text)
		*out_text = IndexNumberArrayGetterBuffer;
	return true;
}

static bool Items_SiteNameGetter(void* data, int idx, const char** out_text)
{
	site_info_t *sites = (site_info_t *) data;

	if(out_text)
		*out_text = _FS_narrow("%s (%s)", sites[idx].name.c_str(), sites[idx].url.c_str());
	return true;
}

int user_info_t::main()
{
	//static int current_title_index = 0;
	static int current_title_status = NU_TITLE_STATUS_NOT_ADDED;

	//std::vector<user_title_info_t>::iterator current_title_it = titles.begin() + current_title_index;

	ImGui::AlignFirstTextHeightToWidgets();
	ImGui::Text("Current site: ");
	ImGui::SameLine();
	int new_current_site = current_site;
	if(ImGui::Combo("##current_site", &new_current_site, Items_SiteNameGetter, sites[0], sites.size()))
		if(new_current_site != current_site)
		{
			current_title_index = 0;

			for(uint32_t i = 0; i < site_users[new_current_site].user_titles.size(); ++i)
				if(sites[site_users[new_current_site].site_index]->titles[site_users[new_current_site].user_titles[i].index].name == 
				   sites[site_users[    current_site].site_index]->titles[site_users[    current_site].user_titles[current_title_index].index].name)
				{
					current_title_index = i;
				}

			current_site = new_current_site;
		}

	ImGui::Separator();

	title_ui(current_title_status);

	ImGui::Separator();

	current_title_index = titlelist_ui();

	ImGui::Separator();

	search_ui();

	return current_title_index;
}

void user_info_t::search_ui()
{
	static char search_str[64] = "";
	//static std::vector<title_info_t> found_titles;
	static int current_found_title_index = 0;

	if(ImGui::InputText("##search", search_str, countof(search_str), ImGuiInputTextFlags_EnterReturnsTrue) || (ImGui::SameLine(), ImGui::Button("Search")))
	{
		last_found_titles.clear();
		if(strlen(search_str) > 0)
			search_title(search_str, last_found_titles);
	}
	ImGui::SameLine();
	if(ImGui::Button("Add"))
		if(!last_found_titles.empty() && !last_found_titles[current_found_title_index].name.empty() && !last_found_titles[current_found_title_index].uri.empty()/* && !site_users[current_site].user_titles.find()*/)
			add_title(last_found_titles[current_found_title_index]);

	//if(!last_found_titles.empty())
	//{
	if(ImGui::ListBox("##empty", &current_found_title_index, Items_TitleGetter, &last_found_titles.begin(), Items_TitleStylerBegin, this, Items_TitleStylerEnd, this, last_found_titles.size(), 10))
		strcpy(search_str, last_found_titles[current_found_title_index].name.c_str());
	//}

	//ImGui::PushItemWidth(-1);
	//ImGui::ListBox("##empty", &current_title_index, Items_TitleMapGetter, &titles.begin(), titles.size());
	//ImGui::PopItemWidth();
}

static inline float  ImLerp(float a, float b, float t)                          { return a + (b - a) * t; }

static inline ImVec4 ImLerp(const ImVec4& a, const ImVec4& b, float t)  { return ImVec4(ImLerp(a.x, b.x, t), ImLerp(a.y, b.y, t), ImLerp(a.z, b.z, t), 1.0f); }

int user_info_t::titlelist_ui()
{
	ImVec4 selected_color(1.0f, 0.5f, 0.5f, 1.0f), normal_color(1.0f, 1.0f, 1.0f, 1.0f);

	for(uint32_t si = 0; si < site_users.size(); ++si)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, current_site == si ? selected_color : normal_color);

		if(ImGui::Checkbox(sites[site_users[si].site_index]->name.c_str(), &site_users[si].enabled))
			set_current_site_next_to(si);

		ImGui::PopStyleColor();

		if(sites.size() > 1 && si < site_users.size() - 1)
			ImGui::SameLine();
	}

	for(uint32_t si = 0; si < site_users.size(); ++si)
		if(site_users[si].enabled)
		{
			//ImGui::PushStyleColor(ImGuiCol_Text, current_site == si ? selected_color : normal_color);

			ImGui::PushStyleColor(ImGuiCol_Button, sites[site_users[si].site_index]->color);

			if(ImGui::Button(("Sync with " + sites[site_users[si].site_index]->url).c_str()))
				for(uint32_t k = 0; k < site_users.size(); ++k)
					if(k != si)
						if(sync(si, k))
							current_site = si;

			ImGui::PopStyleColor();

			//ImGui::PopStyleColor();

			//if(sites.size() > 1 && si < site_users.size() - 1)
				ImGui::SameLine();
		}

	if(ImGui::Button("Sync all"))
		sync_all();

	int numItems = 0; //user_titles.size(); //gui->pack.manifest->numPaths;

	for(uint32_t si = 0; si < site_users.size(); ++si)
		if(site_users[si].enabled)
		{
			numItems += site_users[si].user_titles.size();

			for(uint32_t i = 0; i < site_users[si].user_titles.size(); ++i)
				for(uint32_t k = 0; k < site_users.size(); ++k)
					if(k != si && site_users[k].enabled)
						if(has_title(k, si, i) && k < si)
							--numItems;
		}

	float w = ImGui::GetWindowWidth();

	static bool ColumnsInitialized = false;
	static bool current_sort_mode_descending = false;
	//gui->selectedTab = -1;
	static void* current_compare_func = 0;//indexCompare;
	/*static*/ int selected_row = current_title_index; //numItems > 0 ? 0 : -1;
	static int listItemHovered = -1;

	const int COLUMN_NUM = 7;
	const char *cn[COLUMN_NUM] = { "ID", "Title", "Year", "Type", "Score", "Average score", "Last updated" };

	static ImGuiColumn columns[COLUMN_NUM];

	for(int i = 1; i < countof(columns); ++i)
	{
		columns[i].display_offset = i * w / countof(columns);
	}

	for(int i = 0; i < countof(columns); ++i)
	{
		columns[i].title = cn[i];
	}

	ImGui::Columns(countof(columns), "header");

	//SendColumnOffsetsToImGui(gui);
	const float COLUMN_HEADER_OFFSET = 8.0f;

	for(int i = 1; i < countof(columns); ++i)
	{
		ImGui::SetColumnOffset(i, columns[i].display_offset + COLUMN_HEADER_OFFSET);
	}

	for(int i = 0; i < countof(columns); ++i)
	{
		bool sortSelected = (current_compare_func == columns[i].compare_func);

		if(ImGuiSelectable(false, false, (columns[i].title + std::string(" ") + GetSortDirectionLabel(sortSelected, current_sort_mode_descending)).c_str()))
		{
			current_sort_mode_descending ^= sortSelected ? true : current_sort_mode_descending;
			current_compare_func = columns[i].compare_func;
		}

		//ImGui::Separator();
		ImGui::NextColumn();
	}

	ImGui::Columns(1);
	ImGui::BeginChild("##scrolling", ImVec2(0.0f, 500.0f));
	ImGui::Columns(countof(columns), "contents");

	if(!ColumnsInitialized)
	{
		//SendColumnOffsetsToImGui(gui);
		for(int i = 1; i < countof(columns); ++i)
		{
			ImGui::SetColumnOffset(i, columns[i].display_offset + COLUMN_HEADER_OFFSET);
		}
		ColumnsInitialized = true;
	}

	//GetColumnOffsetsFromImGui(gui);
	for(int i = 1; i < countof(columns); ++i)
	{
		columns[i].display_offset = ImGui::GetColumnOffset(i);
	}

	//DisplayIndex *displayIndices = PushArrayFront(&gui->imguiMemory, DisplayIndex, numItems);

	// Set up the display indices for sorting.
	//for(int i = 0; i < numItems; ++i)
	//{
	//	displayIndices[i].manifest = gui->pack.manifest;
	//	displayIndices[i].index = i;
	//}

	////qsort(displayIndices, numItems, SIZEOF_ARRAY_ELEMENT(displayIndices), current_compare_func);
	//std::sort();

	//if(current_sort_mode_descending)
	//{
	//	int lo = 0, hi = numItems - 1;

	//	while(lo < hi)
	//	{
	//		DisplayIndex temp = displayIndices[lo];

	//		displayIndices[lo] = displayIndices[hi];
	//		displayIndices[hi] = temp;
	//		++lo;
	//		--hi;
	//	}
	//}

	float itemHeight = ImGui::GetTextLineHeightWithSpacing();
	int displayStart = 0, displayEnd = numItems;

	ImGui::CalcListClipping(numItems, itemHeight, &displayStart, &displayEnd);

	for(int i = 0; i < countof(columns); ++i)
	{
		uint32_t site_user_index = 0;
		while(!site_users[site_user_index].enabled)
		{
			++site_user_index;
			assert(site_user_index < site_users.size());
		}
		uint32_t user_title_index = 0;
		uint32_t user_titles_num = site_users[site_user_index].user_titles.size();

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (displayStart * itemHeight));

		for(int k = displayStart; k < displayEnd; ++k)
		{
			int index_selected = 0;//displayIndices[k].index;

			char buffer[128];
			//const char *path = GetResourcePath(editResources[k]);
			//snprintf(buffer, sizeof(buffer), !IsResourceDirty(editResources[k]) ? "%d. %s" : "%d. * %s", k + 1, path);
			//ImGui::PushID(i * k);
			//sprintf(buffer, "Cell %d x %d##%d", i, k, i * k);
			std::string cell_str;

			//uint32_t site_user_index  = k / site_users.size();
			//uint32_t user_title_index = k % site_users.size();

			switch(i)
			{
				case 0: cell_str = std::to_string(sites[site_users[site_user_index].site_index]->titles[site_users[site_user_index].user_titles[user_title_index].index].id); break;
				case 1: cell_str = sites[site_users[site_user_index].site_index]->titles[site_users[site_user_index].user_titles[user_title_index].index].name; break;
				case 2: cell_str = std::to_string(sites[site_users[site_user_index].site_index]->titles[site_users[site_user_index].user_titles[user_title_index].index].year); break;
				case 3: cell_str = sites[site_users[site_user_index].site_index]->title_types[sites[site_users[site_user_index].site_index]->title_type_ids[sites[site_users[site_user_index].site_index]->titles[site_users[site_user_index].user_titles[user_title_index].index].type]]; break;
				case 4: cell_str = Poco::NumberFormatter::format(site_users[site_user_index].user_titles[user_title_index].rating * sites[site_users[site_user_index].site_index]->rating_mulcoef); break;
				case 5: cell_str = Poco::NumberFormatter::format(sites[site_users[site_user_index].site_index]->titles[site_users[site_user_index].user_titles[user_title_index].index].average_rating * sites[site_users[site_user_index].site_index]->rating_mulcoef); break;
				case 6: cell_str = site_users[site_user_index].user_titles[user_title_index].last_updated == Poco::Timestamp::TIMEVAL_MIN ? "N/A" : Poco::DateTimeFormatter::format(site_users[site_user_index].user_titles[user_title_index].last_updated, Poco::DateTimeFormat::SORTABLE_FORMAT); break;
			}

			bool selected = /*current_title_index == k*/ current_site == site_user_index && current_title_index == user_title_index; // current_title_index == site_users[site_user_index].user_titles[user_title_index].index;
			sprintf(buffer, "%s##cell%dx%d", cell_str.c_str(), i, k);
			ImGui::PushStyleColor(ImGuiCol_Text, selected ? selected_color : normal_color);

			ImVec4 color = sites[site_users[site_user_index].site_index]->color;

			if(selected || listItemHovered == k)
				color = ImVec4(0.40f, 0.40f, 0.90f, 0.45f);
			else
				for(uint32_t si = 0; si < site_users.size(); ++si)
					if(si != site_user_index && site_users[si].enabled)
						if(has_title(si, site_user_index, user_title_index))
							color = ImLerp(color, sites[site_users[si].site_index]->color, 0.5f);

			ImGui::PushStyleColor(ImGuiCol_Header, color);

			if(ImGui::Selectable(buffer, true/*selected_row != k && listItemHovered != k*/))
			{
				//ResourceEditGuiSetSelected(gui, k);
				selected_row = k;
				//current_title_index = selected_row;
				current_site = site_user_index;
				current_title_index = user_title_index;//site_users[site_user_index].user_titles[user_title_index].index;
			}

			ImGui::PopStyleColor();

			ImGui::PopStyleColor();

			if(ImGui::IsItemHoveredRect())
			{
				listItemHovered = k;
			}

			//ImGui::PopID();

			//ImGui::PushID(k);

			//if(ImGuiSelectable(false, k == selected_row, "%d", index_selected))
			//{
			//	selected_row = k;
			//	//gui->selectedResource = PackFileRead(&gui->pack, gui->pack.manifest->paths[index_selected], gui->onLoaded, gui->callbackData);
			//	current_title_index = selected_row;
			//}

			//ImGui::PopID();

			bool skip_title = false;

			do 
			{
				++user_title_index;
				if(user_title_index >= user_titles_num)
				{
					user_title_index = 0;
					do 
					{
						++site_user_index;
						if(site_user_index >= site_users.size())
							site_user_index = 0;
					} while(!site_users[site_user_index].enabled);
					user_titles_num = site_users[site_user_index].user_titles.size();
				}

				skip_title = false;

				if(has_title(current_site, site_user_index, user_title_index) && current_site != site_user_index)
					skip_title = true;
				else for(uint32_t si = 0; si < site_users.size(); ++si)
					if(si != site_user_index && site_users[si].enabled)
						if(has_title(si, site_user_index, user_title_index))
							if(si < site_user_index && current_site != site_user_index)
								skip_title = true;
			} while(skip_title);
		}

		ImGui::NextColumn();
	}

	ImGui::Columns(1);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ((numItems - displayEnd) * itemHeight));
	ImGui::EndChild();

	return current_title_index;
}

void user_info_t::title_ui(int &current_title_status)
{
	Poco::FastMutex::ScopedLock lock(mutex);

	if(sites[site_users[current_site].site_index]->titles.empty())
		return;

	if(!sites[site_users[current_site].site_index]->titles[site_users[current_site].user_titles[current_title_index].index].cover_thumb_uri.empty())
	{
		if(sites[site_users[current_site].site_index]->titles[site_users[current_site].user_titles[current_title_index].index].cover_texture_data.empty())
		{
			sites[site_users[current_site].site_index]->titles[site_users[current_site].user_titles[current_title_index].index].cover_texture_data = sites[site_users[current_site].site_index]->http->go_to(sites[site_users[current_site].site_index]->titles[site_users[current_site].user_titles[current_title_index].index].cover_thumb_uri, sites[site_users[current_site].site_index]->login_cookie);
		}

		if(!sites[site_users[current_site].site_index]->titles[site_users[current_site].user_titles[current_title_index].index].cover_texture_data.empty() && !sites[site_users[current_site].site_index]->titles[site_users[current_site].user_titles[current_title_index].index].cover_texture.handle)
		{
			ImplDX9_LoadImage(sites[site_users[current_site].site_index]->titles[site_users[current_site].user_titles[current_title_index].index].cover_texture_data.c_str(), sites[site_users[current_site].site_index]->titles[site_users[current_site].user_titles[current_title_index].index].cover_texture_data.size(), sites[site_users[current_site].site_index]->titles[site_users[current_site].user_titles[current_title_index].index].cover_texture);
		}
	}

	ImVec2 tex_screen_pos = ImGui::GetCursorScreenPos();
	float tex_w = get_cover_width(current_title_index);
	float tex_h = get_cover_height(current_title_index);
	ImTextureID tex_id = sites[site_users[current_site].site_index]->titles[site_users[current_site].user_titles[current_title_index].index].cover_texture.handle; //ImGui::GetIO().Fonts->TexID;
	if(tex_id)
	{
		ImGui::Image(tex_id, ImVec2(tex_w, tex_h), ImVec2(0,0), ImVec2(1,1), ImColor(0,0,0,255), ImColor(255,255,255,128));
		if(ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			float focus_sz = 32.0f;
			float focus_x = clampf(ImGui::GetMousePos().x - tex_screen_pos.x - focus_sz * 0.5f, 0.0f, tex_w - focus_sz);
			float focus_y = clampf(ImGui::GetMousePos().y - tex_screen_pos.y - focus_sz * 0.5f, 0.0f, tex_h - focus_sz);
			ImGui::Text("Min: (%.2f, %.2f)", focus_x, focus_y);
			ImGui::Text("Max: (%.2f, %.2f)", focus_x + focus_sz, focus_y + focus_sz);
			ImVec2 uv0 = ImVec2((focus_x) / tex_w, (focus_y) / tex_h);
			ImVec2 uv1 = ImVec2((focus_x + focus_sz) / tex_w, (focus_y + focus_sz) / tex_h);
			ImGui::Image(tex_id, ImVec2(128,128), uv0, uv1, ImColor(0,0,0,255), ImColor(255,255,255,128));
			ImGui::EndTooltip();
		}
		ImGui::SameLine();
	}

	ImGui::BeginGroup();
	ImGui::PushItemWidth(-1);

	ImGui::TextColored(ImColor(255, 0, 0), "%s (%d)", sites[site_users[current_site].site_index]->titles[site_users[current_site].user_titles[current_title_index].index].name.c_str(), sites[site_users[current_site].site_index]->titles[site_users[current_site].user_titles[current_title_index].index].year);
	ImGui::Separator();
	//if(user_titles[current_title_index].status != NU_TITLE_STATUS_NOT_ADDED && user_titles[current_title_index].status != NU_TITLE_STATUS_PLAN_TO_WATCH)
	{
		ImGui::Text("Type: %s", sites[site_users[current_site].site_index]->title_types[sites[site_users[current_site].site_index]->title_type_ids[sites[site_users[current_site].site_index]->titles[site_users[current_site].user_titles[current_title_index].index].type]].c_str());
		if(sites[site_users[current_site].site_index]->titles[site_users[current_site].user_titles[current_title_index].index].episodes_num > 1)
		{
			ImGui::AlignFirstTextHeightToWidgets();
			ImGui::Text("Episodes: ");
			ImGui::SameLine();
			int current_title_episodes_watched_num = (int) site_users[current_site].user_titles[current_title_index].episodes_watched_num;
			const char *current_title_episodes_watched_num_str = 0;
			Items_IndexNumberArrayGetter(0, current_title_episodes_watched_num, &current_title_episodes_watched_num_str);
			ImGui::PushItemWidth(ImGui::CalcTextSize(current_title_episodes_watched_num_str).x + ImGui::GetStyle().ItemInnerSpacing.x + ImGui::GetStyle().FramePadding.x + ImGui::GetWindowFontSize() + ImGui::GetStyle().FramePadding.x * 2.0f);
			if(ImGui::Combo("##episodes_watched_num", &current_title_episodes_watched_num, Items_IndexNumberArrayGetter, 0, sites[site_users[current_site].site_index]->titles[site_users[current_site].user_titles[current_title_index].index].episodes_num + 1))
			{
				set_title_episodes_watched_num(current_title_index, current_title_episodes_watched_num);
			}
			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::Text(" of %d", sites[site_users[current_site].site_index]->titles[site_users[current_site].user_titles[current_title_index].index].episodes_num);
		}
		else
			ImGui::Text("Episodes: %d of %d", site_users[current_site].user_titles[current_title_index].episodes_watched_num, sites[site_users[current_site].site_index]->titles[site_users[current_site].user_titles[current_title_index].index].episodes_num);
		ImGui::SameLine();
		if(ImGui::Button("+") && site_users[current_site].user_titles[current_title_index].episodes_watched_num < sites[site_users[current_site].site_index]->titles[site_users[current_site].user_titles[current_title_index].index].episodes_num)
		{
			uint32_t current_title_episodes_watched_num = site_users[current_site].user_titles[current_title_index].episodes_watched_num + 1;

			set_title_episodes_watched_num(current_title_index, current_title_episodes_watched_num);
		}
		ImGui::SameLine();
		if(ImGui::Button("-") && site_users[current_site].user_titles[current_title_index].episodes_watched_num > 0)
		{
			uint32_t current_title_episodes_watched_num = site_users[current_site].user_titles[current_title_index].episodes_watched_num - 1;

			set_title_episodes_watched_num(current_title_index, current_title_episodes_watched_num);
		}
		//ImGui::SameLine();
		//ImGui::SliderInt("##eps", (int *)&user_titles[current_title_index].episodes_watched_num, 0, sites[current_site].titles[current_title_index].episodes_num);
		ImGui::Text("Times watched: %d", site_users[current_site].user_titles[current_title_index].times_watched_num);

	ImGui::AlignFirstTextHeightToWidgets();
	//ImGui::Text("Status: %s", sites[current_site].title_statuses[user_titles[current_title_index].status].c_str());
	ImGui::Text("Status: ");
	ImGui::SameLine();
	current_title_status = (int) site_users[current_site].user_titles[current_title_index].status;
	if(ImGui::Combo("##status", &current_title_status, &sites[site_users[current_site].site_index]->title_statuses[0], sites[site_users[current_site].site_index]->title_statuses.size()))
		if(sites[site_users[current_site].site_index]->send_request_change_title_status(site_users[current_site], site_users[current_site].user_titles[current_title_index].index, current_title_status))
		{
			site_users[current_site].user_titles[current_title_index].status = current_title_status;

			if(site_users[current_site].user_titles[current_title_index].status == NU_TITLE_STATUS_NOT_ADDED)
				current_title_index = sites[site_users[current_site].site_index]->remove_title(site_users[current_site], current_title_index);

			//if(user_titles[current_title_index].status == NU_TITLE_STATUS_WATCHED)
			//	ask_to_write_review();
		}
	//ImGui::Text("Score: %d of 10", user_titles[current_title_index].rating * sites[current_site].rating_mulcoef);
	ImGui::AlignFirstTextHeightToWidgets();
	ImGui::Text("Score: ");
	ImGui::SameLine();
	if(site_users[current_site].user_titles[current_title_index].episodes_watched_num < 1)
		ImGui::Text("must watch at least one episode to rate.");
	else
	{
		int current_title_rating_max = 10;
		float current_title_rating = site_users[current_site].user_titles[current_title_index].rating * sites[site_users[current_site].site_index]->rating_mulcoef;
		int current_title_rating_index = floorf(current_title_rating + 0.5f);
		const char *current_title_rating_str = 0;
		Items_IndexNumberArrayGetter(0, current_title_rating, &current_title_rating_str);
		ImGui::PushItemWidth(ImGui::CalcTextSize(current_title_rating_str).x + ImGui::GetStyle().ItemInnerSpacing.x + ImGui::GetStyle().FramePadding.x + ImGui::GetWindowFontSize() + ImGui::GetStyle().FramePadding.x * 2.0f);
		if(ImGui::Combo("##rating", &current_title_rating_index, Items_IndexNumberArrayGetter, 0, current_title_rating_max + 1))
		{
			current_title_rating = (float) current_title_rating_index;
			if(sites[site_users[current_site].site_index]->send_request_change_title_rating(site_users[current_site], site_users[current_site].user_titles[current_title_index].index, current_title_rating))
				site_users[current_site].user_titles[current_title_index].rating = current_title_rating / sites[site_users[current_site].site_index]->rating_mulcoef;
		}
		ImGui::PopItemWidth();
	}
	ImGui::Text("Average score: %.3f of 10 (%d votes, rank #%d)", sites[site_users[current_site].site_index]->titles[site_users[current_site].user_titles[current_title_index].index].average_rating * sites[site_users[current_site].site_index]->rating_mulcoef,
																		 sites[site_users[current_site].site_index]->titles[site_users[current_site].user_titles[current_title_index].index].votes_num,
																		 sites[site_users[current_site].site_index]->titles[site_users[current_site].user_titles[current_title_index].index].rank);
	}

	ImGui::PopItemWidth();
	ImGui::EndGroup();
}

float user_info_t::get_cover_width(uint32_t i)
{
	return sites[site_users[current_site].site_index]->titles[site_users[current_site].user_titles[i].index].cover_texture.w * sites[site_users[current_site].site_index]->cover_image_scale_x;
}

float user_info_t::get_cover_height(uint32_t i)
{
	return sites[site_users[current_site].site_index]->titles[site_users[current_site].user_titles[i].index].cover_texture.h * sites[site_users[current_site].site_index]->cover_image_scale_y;
}

uint32_t user_info_t::get_user_title_index(uint32_t si, const std::string &title_name)
{
	for(uint32_t i = 0; i < site_users[si].user_titles.size(); ++i)
		if(sites[site_users[si].site_index]->titles[site_users[si].user_titles[i].index].name == title_name)
			return i;

	return GW_NOT_FOUND;
}

uint32_t user_info_t::get_user_title_index_parse(uint32_t si, const std::string &title_name)
{
	uint32_t title_index = GW_NOT_FOUND;

	for(uint32_t i = 0; i < sites[site_users[si].site_index]->titles.size(); ++i)
		if(sites[site_users[si].site_index]->titles[i].name.c_str() == title_name)
		{
			user_title_info_t user_title = { 0 };

			user_title.index = i;

			if(!sites[site_users[si].site_index]->parse_user_title_info(site_users[si], user_title))
				break;

			site_users[si].user_titles.push_back(user_title);

			title_index = site_users[si].user_titles.size() - 1;

			break;
		}

	return title_index;
}

uint32_t user_info_t::find_and_add_title(uint32_t si, const std::string &title_name)
{
	uint32_t title_index = GW_NOT_FOUND;

	last_found_titles.clear();
	if(search_title(si, title_name, last_found_titles))
		for(uint32_t i = 0; i <  last_found_titles.size(); ++i)
			if(last_found_titles[i].name == title_name) // exact match
			{
				if(add_title(si, last_found_titles[i]))
					title_index = site_users[si].user_titles.size() - 1;

				break;
			}

	return title_index;
}

bool user_info_t::authenticate()
{
	return sites[site_users[current_site].site_index]->authenticate(site_users[current_site]);
}

bool user_info_t::sync(uint32_t i, uint32_t k)
{
	try
	{
		//return sites[site_users[i].site_index]->sync(username, password, site_users[k]);
		return sites[site_users[i].site_index]->sync(username, password, site_users[i]);
	}
	catch(Exception& exc)
	{
		std::cerr << exc.displayText() << std::endl;
		return false;
	}
}

void user_info_t::sync_all()
{
	//for(uint32_t i = 0; i < site_users.size() / 2; ++i)
	//	sync(i, site_users.size() - i - 1);
	for(uint32_t i = 0; i < site_users.size(); ++i)
		sync(i, 0);

	add_to_history(NU_ACT_SYNC_ALL);
}

bool user_info_t::has_title(uint32_t si, uint32_t site_user_index, uint32_t user_title_index)
{
	for(uint32_t i = 0; i < site_users[si].user_titles.size(); ++i)
		if((i != user_title_index || si != site_user_index) && 
			sites[site_users[si             ].site_index]->titles[site_users[si             ].user_titles[i].index].name == 
			sites[site_users[site_user_index].site_index]->titles[site_users[site_user_index].user_titles[user_title_index].index].name)
		{
			return true;
		}

	return false;
}

void user_info_t::set_current_site_next_to(uint32_t si)
{
	if(!site_users[si].enabled && current_site == si)
		if(site_users.size() > 1)
		{
			if(si < site_users.size() - 1 && site_users[si + 1].enabled)
				current_site = si + 1;
			else if(si > 0 && site_users[si - 1].enabled)
				current_site = si - 1;
			current_title_index = 0;
		}
}

void user_info_t::add_to_history(nu_user_action_type user_action_type, const char_t *desc)
{
	action_history.add(user_action_type);

	tcout << _FS(_T("[%s] %s's action: %s %s"), GW_A2T(Poco::DateTimeFormatter::format(action_history.user_actions.back().timestamp, Poco::DateTimeFormat::SORTABLE_FORMAT)), GW_A2T(username), action_history.user_actions.back().to_str(), desc) << std::endl;
}

