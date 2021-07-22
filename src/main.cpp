// https://gist.github.com/ppLorins/6e4cc625c2c5b8fd16ced3172b1ada09

#include <csignal>
#include <iostream>

#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>

using namespace std;

class GSession
{
public:
	std::mutex m_mutex{};
	GSession(uint64_t session_id) : m_session_id(session_id){};
	void Process()
	{
		cout << "Processing session: " << m_session_id << " seq: " << m_seq_num++ << endl;
	};

private:
	uint64_t m_seq_num{0};
	const uint64_t m_session_id{0};
};

class GServer
{
public:
	static GServer &GetInstance()
	{
		static GServer kGlobalInstance; // thread safe only in C++11
		return kGlobalInstance;
	}

	void Run()
	{
		std::thread Thread01([this]
		{
			while (1)
			{
				//std::this_thread::sleep_for(std::chrono::milliseconds(10));

				std::lock_guard<std::mutex> lock_guard{m_mutex_sessions};
				for (const auto &it : m_sessions)
				{
					std::lock_guard<std::mutex> inner_lock_guard{it.second->m_mutex};
					it.second->Process();
				}
			}
		});

		std::thread Thread02([this]
		{
			while (1)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				AddSession();
			}
		});


		if (Thread01.joinable())
		{
			Thread01.join();
		}
		if (Thread02.joinable())
		{
			Thread02.join();
		}
	}

	std::shared_ptr<GSession> AddSession()
	{
		auto new_session = std::make_shared<GSession>(m_next_session_id);

		std::lock_guard<std::mutex> lock_guard{m_mutex_sessions};
		m_sessions[m_next_session_id++] = new_session;
		return new_session;
	};

	std::shared_ptr<GSession> GetSession(uint64_t session_id)
	{
		std::lock_guard<std::mutex> local_lock_guard{m_mutex_sessions};

		auto it = m_sessions.find(session_id);
		if (it == m_sessions.end())
		{
			return nullptr;
		}
		return it->second;		
	};

private:
	GServer() = default;
	virtual ~GServer() = default;

public:
	std::atomic_uint64_t m_next_session_id{0};

	std::mutex m_mutex_sessions{};
	std::unordered_map<uint64_t, std::shared_ptr<GSession>> m_sessions{};
};

void SignalHandler(int signal)
{
	cout << "Received termination signal: " << signal << endl;
	exit(-1);
}

int main(int argc, char **argv)
{
	cout << "Starting Server..." << endl;

	signal(SIGTERM, SignalHandler);
	signal(SIGINT, SignalHandler);
	signal(SIGKILL, SignalHandler);

	try
	{
		GServer::GetInstance().Run();
	}
	catch (std::exception &e)
	{
		cout << "Exception: " << e.what() << endl;
		return -1;
	}

	return 0;
}