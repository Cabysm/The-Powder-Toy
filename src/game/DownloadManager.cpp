#include "DownloadManager.h"
#include "Download.h"
#include "http.h"
#include "defines.h"
#include "common/Platform.h"

DownloadManager::DownloadManager():
	threadStarted(false),
	lastUsed(time(NULL)),
	managerRunning(false),
	managerShutdown(false),
	downloads(std::vector<Download*>()),
	downloadsAddQueue(std::vector<Download*>())
{
	http_init(http_proxy_string[0] ? http_proxy_string : NULL);

	pthread_mutex_init(&downloadLock, NULL);
	pthread_mutex_init(&downloadAddLock, NULL);
}

DownloadManager::~DownloadManager()
{

}

void DownloadManager::Shutdown()
{
	pthread_mutex_lock(&downloadLock);
	pthread_mutex_lock(&downloadAddLock);
	for (std::vector<Download*>::iterator iter = downloads.begin(); iter != downloads.end(); ++iter)
	{
		Download *download = (*iter);
		if (download->http)
			http_force_close(download->http);
		download->downloadCanceled = true;
		delete download;
	}
	downloads.clear();
	downloadsAddQueue.clear();
	managerShutdown = true;
	pthread_mutex_unlock(&downloadAddLock);
	pthread_mutex_unlock(&downloadLock);
	pthread_join(downloadThread, NULL);

	http_done();
}

//helper function for download
TH_ENTRY_POINT void* DownloadManagerHelper(void* obj)
{
	DownloadManager *temp = (DownloadManager*)obj;
	temp->Update();
	return NULL;
}

void DownloadManager::Start()
{
	managerRunning = true;
	lastUsed = time(NULL);
	pthread_create(&downloadThread, NULL, &DownloadManagerHelper, this);
}

void DownloadManager::Update()
{
	unsigned int numActiveDownloads;
	while (!managerShutdown)
	{
		pthread_mutex_lock(&downloadAddLock);
		if (downloadsAddQueue.size())
		{
			for (size_t i = 0; i < downloadsAddQueue.size(); i++)
			{
				downloads.push_back(downloadsAddQueue[i]);
			}
			downloadsAddQueue.clear();
		}
		pthread_mutex_unlock(&downloadAddLock);
		if (downloads.size())
		{
			numActiveDownloads = 0;
			pthread_mutex_lock(&downloadLock);
			for (size_t i = 0; i < downloads.size(); i++)
			{
				Download *download = downloads[i];
				if (download->CheckCanceled())
				{
					if (download->http && download->CheckStarted())
						http_force_close(download->http);
					delete download;
					downloads.erase(downloads.begin()+i);
					i--;
				}
				else if (download->CheckStarted() && !download->CheckDone())
				{
					if (http_async_req_status(download->http) != 0)
					{
						download->downloadData = http_async_req_stop(download->http, &download->downloadStatus, &download->downloadSize);
						download->downloadFinished = true;
						if (!download->keepAlive)
							download->http = NULL;
					}
					lastUsed = time(NULL);
					numActiveDownloads++;
				}
			}
			pthread_mutex_unlock(&downloadLock);
		}
		if (time(NULL) > lastUsed+HTTP_TIMEOUT*2 && !numActiveDownloads)
		{
			pthread_mutex_lock(&downloadLock);
			managerRunning = false;
			pthread_mutex_unlock(&downloadLock);
			return;
		}
		Platform::Millisleep(1);
	}
}

void DownloadManager::EnsureRunning()
{
	pthread_mutex_lock(&downloadLock);
	if (!managerRunning)
	{
		if (threadStarted)
			pthread_join(downloadThread, NULL);
		else
			threadStarted = true;
		Start();
	}
	pthread_mutex_unlock(&downloadLock);
}

void DownloadManager::AddDownload(Download *download)
{
	pthread_mutex_lock(&downloadAddLock);
	downloadsAddQueue.push_back(download);
	pthread_mutex_unlock(&downloadAddLock);
	EnsureRunning();
}

void DownloadManager::Lock()
{
	pthread_mutex_lock(&downloadAddLock);
}

void DownloadManager::Unlock()
{
	pthread_mutex_unlock(&downloadAddLock);
}
