#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <windows.h>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/thread/thread.hpp>
#include <nlohmann/json.hpp>

//#include <cstring>

#include "xcontroller.h"

using boost::asio::ip::tcp;

class session
{
public:
	session(boost::asio::io_service& io_service, controller_data* controller)
		: socket_(io_service),
		controller_(controller)
	{}

	tcp::socket& socket()
	{
		return socket_;
	}

	void start()
	{
		socket_.async_read_some(boost::asio::buffer(data_, max_length),
			boost::bind(&session::handle_read, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
	}

private:
	void handle_read(const boost::system::error_code& error, size_t bytes_transferred)
	{
		DWORD mutexWaitResult;
		DWORD semWaitResult;

		if (!error && controller_->xSuccess!=0)
		{
			mutexWaitResult = WaitForSingleObject(controller_->ghMutex, 0L);
			semWaitResult = WaitForSingleObject(controller_->ghSemaphore, 0L);
			if (!ReleaseMutex(controller_->ghMutex))
			{
				//printf("ReleaseMutex error in client: %d\n", GetLastError());
			}
			nlohmann::json json = *controller_;
			if (!ReleaseSemaphore(controller_->ghSemaphore, 1, NULL))
			{
				//printf("ReleaseMutex error in client: %d\n", GetLastError());
			}
			std::string json_as_string = json.dump();
			boost::asio::async_write(socket_,
				boost::asio::buffer(json_as_string.c_str(), json_as_string.length()),
				boost::bind(&session::handle_write, this,
					boost::asio::placeholders::error));
		}
		else
		{
			delete this;
		}
	}

	void handle_write(const boost::system::error_code& error)
	{
		if (!error)
		{
			socket_.async_read_some(boost::asio::buffer(data_, max_length),
				boost::bind(&session::handle_read, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
	}

	tcp::socket socket_;
	enum { max_length = 1024 };
	char data_[max_length];
	controller_data* controller_;
};

class server
{
public:
	server(boost::asio::io_service& io_service, short port, controller_data* controller)
		:io_service_(io_service),
		acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
		controller_(controller)
	{
		start_accept();
	}
private:
	void start_accept()
	{
		session* new_session = new session(io_service_, controller_);

		acceptor_.async_accept(new_session->socket(),
			boost::bind(&server::handle_accept, this, 
				new_session, boost::asio::placeholders::error));
	}

	void handle_accept(session* new_session, const boost::system::error_code& error)
	{
		if (!error && controller_->xSuccess!=0)
			new_session->start();
		else
			delete new_session;
		
		start_accept();
	}

	boost::asio::io_service& io_service_;
	tcp::acceptor acceptor_;
	controller_data* controller_;
};