/*
    Copyright (c) 2007-2009 FastMQ Inc.

    This file is part of 0MQ.

    0MQ is free software; you can redistribute it and/or modify it under
    the terms of the Lesser GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    0MQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    Lesser GNU General Public License for more details.

    You should have received a copy of the Lesser GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __ZMQ_HPP_INCLUDED__
#define __ZMQ_HPP_INCLUDED__

#include "zmq.h"

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <exception>

namespace zmq
{

    typedef zmq_free_fn free_fn;
    typedef zmq_pollitem_t pollitem_t;

    inline int poll (zmq_pollitem_t *items_, int nitems_)
    {
        return zmq_poll (items_, nitems_);
    }

    class error_t : public std::exception
    {
    public:

        error_t () : errnum (errno) {}

        virtual const char *what () const throw ()
        {
            return zmq_strerror (errnum);
        }

    private:

        int errnum;
    };

    class message_t : private zmq_msg_t
    {
        friend class socket_t;

    public:

        inline message_t ()
        {
            int rc = zmq_msg_init (this);
            if (rc != 0)
                throw error_t ();
        }

        inline message_t (size_t size_)
        {
            int rc = zmq_msg_init_size (this, size_);
            if (rc != 0)
                throw error_t ();
        }

        inline message_t (void *data_, size_t size_, free_fn *ffn_)
        {
            int rc = zmq_msg_init_data (this, data_, size_, ffn_);
            if (rc != 0)
                throw error_t ();
        }

        inline ~message_t ()
        {
            int rc = zmq_msg_close (this);
            if (rc != 0)
                throw error_t ();
        }

        inline void rebuild ()
        {
            int rc = zmq_msg_close (this);
            if (rc != 0)
                throw error_t ();
            rc = zmq_msg_init (this);
            if (rc != 0)
                throw error_t ();
        }

        inline void rebuild (size_t size_)
        {
            int rc = zmq_msg_close (this);
            if (rc != 0)
                throw error_t ();
            rc = zmq_msg_init_size (this, size_);
            if (rc != 0)
                throw error_t ();
        }

        inline void rebuild (void *data_, size_t size_, free_fn *ffn_)
        {
            int rc = zmq_msg_close (this);
            if (rc != 0)
                throw error_t ();
            rc = zmq_msg_init_data (this, data_, size_, ffn_);
            if (rc != 0)
                throw error_t ();
        }

        inline void move (message_t *msg_)
        {
            int rc = zmq_msg_move (this, (zmq_msg_t*) msg_);
            if (rc != 0)
                throw error_t ();
        }

        inline void copy (message_t *msg_)
        {
            int rc = zmq_msg_copy (this, (zmq_msg_t*) msg_);
            if (rc != 0)
                throw error_t ();
        }

        inline void *data ()
        {
            return zmq_msg_data (this);
        }

        inline size_t size ()
        {
            return zmq_msg_size (this);
        }

    private:

        //  Disable implicit message copying, so that users won't use shared
        //  messages (less efficient) without being aware of the fact.
        message_t (const message_t&);
        void operator = (const message_t&);
    };

    class context_t
    {
        friend class socket_t;

    public:

        inline context_t (int app_threads_, int io_threads_, int flags_ = 0)
        {
            ptr = zmq_init (app_threads_, io_threads_, flags_);
            if (ptr == NULL)
                throw error_t ();
        }

        inline ~context_t ()
        {
            int rc = zmq_term (ptr);
            assert (rc == 0);
        }

    private:

        void *ptr;

        context_t (const context_t&);
        void operator = (const context_t&);
    };

    class socket_t
    {
    public:

        inline socket_t (context_t &context_, int type_)
        {
            ptr = zmq_socket (context_.ptr, type_);
            if (ptr == NULL)
                throw error_t ();
        }

        inline ~socket_t ()
        {
            int rc = zmq_close (ptr);
            if (rc != 0)
                throw error_t ();
        }

        inline operator void* ()
        {
            return ptr;
        }

        inline void setsockopt (int option_, const void *optval_,
            size_t optvallen_)
        {
            int rc = zmq_setsockopt (ptr, option_, optval_, optvallen_);
            if (rc != 0)
                throw error_t ();
        }

        inline void bind (const char *addr_)
        {
            int rc = zmq_bind (ptr, addr_);
            if (rc != 0)
                throw error_t ();
        }

        inline void connect (const char *addr_)
        {
            int rc = zmq_connect (ptr, addr_);
            if (rc != 0)
                throw error_t ();
        }

        inline bool send (message_t &msg_, int flags_ = 0)
        {
            int rc = zmq_send (ptr, &msg_, flags_);
            if (rc == 0)
                return true;
            if (rc == -1 && errno == EAGAIN)
                return false;
            throw error_t ();
        }

        inline void flush ()
        {
            int rc = zmq_flush (ptr);
            if (rc != 0)
                throw error_t ();
        }

        inline bool recv (message_t *msg_, int flags_ = 0)
        {
            int rc = zmq_recv (ptr, msg_, flags_);
            if (rc == 0)
                return true;
            if (rc == -1 && errno == EAGAIN)
                return false;
            throw error_t ();
        }

    private:

        void *ptr;

        socket_t (const socket_t&);
        void operator = (const socket_t&);
    };

}

#endif