//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//
#include "websocket_session.hpp"
#include <iostream>
#include "Auth.h"

websocket_session::
websocket_session(
    tcp::socket&& socket,
    boost::shared_ptr<shared_state> const& state)
    : ws_(std::move(socket))
    , state_(state)
{
}

websocket_session::
~websocket_session()
{
    // Remove this session from the list of active sessions
    state_->leave(this);
}

void
websocket_session::
fail(beast::error_code ec, char const* what)
{
    // Don't report these
    if (ec == net::error::operation_aborted ||
        ec == websocket::error::closed)
        return;

    std::cerr << what << ": " << ec.message() << "\n";
}

void
websocket_session::
on_accept(beast::error_code ec)
{
    // Handle the error, if any
    if (ec)
        return fail(ec, "accept");

    // Add this session to the list of active sessions
    state_->join(this);

    // Read a message
    ws_.async_read(
        buffer_,
        beast::bind_front_handler(
            &websocket_session::on_read,
            shared_from_this()));
}

extern SociDb psql;

void
websocket_session::
on_read(beast::error_code ec, std::size_t)
{
    SociDb* Db = &psql;
    //psql.ConnectToDB();
    // Handle the error, if any
    if (ec)
        return fail(ec, "read");

    std::string buffData = beast::buffers_to_string(buffer_.data());
    std::string reg = buffData.substr(0, 10);
    std::string response;
    std::string username;
    std::string password;
    std::size_t namePos;
    std::size_t passPos;

    if (reg == ":register:") {
        namePos = 10; //":register:"
        passPos = buffData.find(":password:") + 10;
        username = buffData.substr(10, passPos - 20);
        password = buffData.substr(passPos);

        //method addUser returns true if username is not user by other person and adds user to DB
        if (Db->addUser(username, password)) {
            response = "new user registered: " + username;
            state_->send(response);
        }
    }
    else {
        namePos = 10; //:username:
        passPos = buffData.find(":password:") + 10;
        std::size_t messPos = buffData.find(":message:") + 9;

        username = buffData.substr(10, passPos - 20);
        password = buffData.substr(passPos, messPos - passPos - 9);
        std::string message = buffData.substr(messPos);

        //check for user in Db and send message only if exists
        if (Db->GetUser(username, password)) {
            response = username + ": " + message;
            // Send to all connections
            state_->send(response);
        }
    }
    // Clear the buffer  
    buffer_.consume(buffer_.size());


    // Read another message
    ws_.async_read(
        buffer_,
        beast::bind_front_handler(
            &websocket_session::on_read,
            shared_from_this()));
}

void
websocket_session::
send(boost::shared_ptr<std::string const> const& ss)
{
    // Post our work to the strand, this ensures
    // that the members of `this` will not be
    // accessed concurrently.

    net::post(
        ws_.get_executor(),
        beast::bind_front_handler(
            &websocket_session::on_send,
            shared_from_this(),
            ss));
}

void
websocket_session::
on_send(boost::shared_ptr<std::string const> const& ss)
{
    // Always add to queue
    queue_.push_back(ss);

    // Are we already writing?
    if (queue_.size() > 1)
        return;

    // We are not currently writing, so send this immediately
    ws_.async_write(
        net::buffer(*queue_.front()),
        beast::bind_front_handler(
            &websocket_session::on_write,
            shared_from_this()));
}

void
websocket_session::
on_write(beast::error_code ec, std::size_t)
{
    // Handle the error, if any
    if (ec)
        return fail(ec, "write");

    // Remove the string from the queue
    queue_.erase(queue_.begin());

    // Send the next message if any
    if (!queue_.empty())
        ws_.async_write(
            net::buffer(*queue_.front()),
            beast::bind_front_handler(
                &websocket_session::on_write,
                shared_from_this()));
}