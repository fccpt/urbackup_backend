/*************************************************************************
*    UrBackup - Client/Server backup system
*    Copyright (C) 2011-2016 Martin Raiber
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU Affero General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/
#include "server_update.h"
#include "../urlplugin/IUrlFactory.h"
#include "../Interface/Server.h"
#include "../Interface/File.h"
#include "../stringtools.h"
#include <stdlib.h>
#include <memory>

extern IUrlFactory *url_fak;

namespace
{
	std::string urbackup_update_url = "http://update3.urbackup.org/";

	struct SUpdatePlatform
	{
		SUpdatePlatform(std::string extension,
			std::string basename, std::string versionname)
			: extension(extension), basename(basename),
			versionname(versionname)
		{}
		std::string extension;
		std::string basename;
		std::string versionname;
	};
}

ServerUpdate::ServerUpdate(void)
{
}

void ServerUpdate::update_client()
{
	if(url_fak==NULL)
	{
		Server->Log("Urlplugin not found. Cannot download client for autoupdate.", LL_ERROR);
		return;
	}

	read_update_location();

	std::string http_proxy = Server->getServerParameter("http_proxy");

	std::vector<SUpdatePlatform> update_files;

	update_files.push_back(SUpdatePlatform("exe", "UrBackupUpdate", "version.txt"));
	update_files.push_back(SUpdatePlatform("sh", "UrBackupUpdateMac", "version_osx.txt"));
	update_files.push_back(SUpdatePlatform("sh", "UrBackupUpdateLinux", "version_linux.txt"));

	for (size_t i = 0; i < update_files.size(); ++i)
	{
		SUpdatePlatform& curr = update_files[i];

		std::string errmsg;
		Server->Log("Downloading version file...", LL_INFO);
		std::string version = url_fak->downloadString(urbackup_update_url + curr.versionname, http_proxy, &errmsg);
		if (version.empty())
		{
			Server->Log("Error while downloading version info from " + urbackup_update_url + curr.versionname + ": " + errmsg, LL_ERROR);
			return;
		}
		std::string curr_version = getFile("urbackup/"+curr.versionname);
		if (curr_version.empty()) curr_version = "0";

		if (version!=curr_version)
		{
			Server->Log("Downloading signature...", LL_INFO);

			IFile* sig_file = Server->openFile("urbackup/" + curr.basename + ".sig2", MODE_WRITE);
			if (sig_file == NULL)
			{
				Server->Log("Error opening signature output file urbackup/" + curr.basename + ".sig2", LL_ERROR);
				return;
			}
			ObjectScope sig_file_scope(sig_file);

			bool b = url_fak->downloadFile(urbackup_update_url + curr.basename + ".sig2", sig_file, http_proxy, &errmsg);

			if (!b)
			{
				Server->Log("Error while downloading update signature from " + urbackup_update_url + curr.basename + ".sig2: " + errmsg, LL_ERROR);
			}

			if (curr.extension == "exe")
			{
				Server->Log("Downloading old signature...", LL_INFO);

				IFile* old_sig_file = Server->openFile("urbackup/" + curr.basename + ".sig", MODE_WRITE);
				if (old_sig_file == NULL)
				{
					Server->Log("Error opening signature output file urbackup/" + curr.basename + ".sig", LL_ERROR);
					return;
				}
				ObjectScope old_sig_file_scope(old_sig_file);

				bool b = url_fak->downloadFile(urbackup_update_url + curr.basename + ".sig", old_sig_file, http_proxy, &errmsg);

				if (!b)
				{
					Server->Log("Error while downloading old update signature from " + urbackup_update_url + curr.basename + ".sig: " + errmsg, LL_ERROR);
				}
			}

			Server->Log("Getting update file URL...", LL_INFO);
			std::string update_url = url_fak->downloadString(urbackup_update_url + curr.basename + ".url", http_proxy, &errmsg);

			if (update_url.empty())
			{
				Server->Log("Error while downloading update url from " + urbackup_update_url + curr.basename + ".url: " + errmsg, LL_ERROR);
				return;
			}

			IFile* update_file = Server->openFile("urbackup/" + curr.basename + "." + curr.extension, MODE_WRITE);
			if (update_file == NULL)
			{
				Server->Log("Error opening update output file urbackup/" + curr.basename + "." + curr.extension, LL_ERROR);
				return;
			}
			ObjectScope update_file_scope(update_file);

			Server->Log("Downloading update file...", LL_INFO);
			b = url_fak->downloadFile(update_url, update_file, http_proxy, &errmsg);

			if (!b)
			{
				Server->Log("Error while downloading update file from " + update_url + ": " + errmsg, LL_ERROR);
				return;
			}

			sig_file->Sync();
			update_file->Sync();

			Server->Log("Successfully downloaded update file.", LL_INFO);
			writestring(version, "urbackup/"+curr.versionname);
		}
	}
}

void ServerUpdate::update_server_version_info()
{
	if(url_fak==NULL)
	{
		Server->Log("Urlplugin not found. Cannot download server version info.", LL_ERROR);
		return;
	}

	read_update_location();

	std::string http_proxy = Server->getServerParameter("http_proxy");

	std::string errmsg;
	Server->Log("Downloading server version info...", LL_INFO);

	std::auto_ptr<IFile> server_version_info(Server->openFile("urbackup/server_version_info.properties", MODE_WRITE));

	if(!server_version_info.get())
	{
		Server->Log("Error opening urbackup/server_version_info.properties for writing", LL_ERROR);
	}
	else
	{
		if(!url_fak->downloadFile(urbackup_update_url+"server_version_info.properties", 
			server_version_info.get(), http_proxy, &errmsg) )
		{
			Server->Log("Error downloading server version information: " + errmsg, LL_ERROR);
		}
	}	
}

void ServerUpdate::read_update_location()
{
	std::string read_update_location = trim(getFile("urbackup/server_update_location.url"));

	if (!read_update_location.empty())
	{
		urbackup_update_url = read_update_location;
	}
}
