/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */


#pragma once


#include <errno.h>
#ifndef HAVE_PRECOMPILED_HEADERS
#include <string>
#include <cstring>
#include <exception>
#include <thread>
#include <atomic>
#include <mutex>
#endif 
#ifndef _WIN32
#include <poll.h>
#endif
#include <libssh/callbacks.h>
#include "base/threading.h"

#ifndef NOEXCEPT
#if defined(_WIN32) || defined(__APPLE__)
#define NOEXCEPT _NOEXCEPT
#else
#ifndef _GLIBCXX_USE_NOEXCEPT
#define NOEXCEPT throw()
#else
#define NOEXCEPT _GLIBCXX_USE_NOEXCEPT
#endif
#endif
#endif

#ifdef _WIN32
typedef int socklen_t;
#endif

struct ssh_threads_callbacks_struct * ssh_threads_get_std_threads(void);
void sshLogCallback(int priority, const char *function, const char *buffer, void *userdata);

namespace ssh {

  inline void wbCloseSocket(int socket) {
#if _MSC_VER
    closesocket(socket);
#else
    close(socket);
#endif
  }

  inline int wbPoll(pollfd *data, size_t size) {
#if _MSC_VER
    return WSAPoll(data, size, -1);
#else
    return poll(data, (nfds_t)size, -1);
#endif
  }

  const std::size_t LOG_SIZE_1MB = 1048576;
  static std::once_flag sshInitOnce;
  std::string getError();
  std::string getSftpErrorDescription(int rc);
  void setSocketNonBlocking(int sock);
  void initLibSSH();

  class SSHConnectionConfig {
  public:
    SSHConnectionConfig();
    std::string localhost;
    int localport;
    ssize_t bufferSize;
    std::string remoteSSHhost;
    std::size_t remoteSSHport;
    std::string remotehost;
    int remoteport;
    bool strictHostKeyCheck;
    int compressionLevel;
    std::string fingerprint;
    std::string configFile;
    std::string knownHostsFile;
    std::string optionsDir;
    std::size_t connectTimeout;
    std::size_t readWriteTimeout;
    std::size_t commandTimeout;
    std::size_t commandRetryCount;
    std::string getServer() {
      return remotehost + ":" + std::to_string(remoteport);
    }

    void dumpConfig() const;
    friend bool operator==(const SSHConnectionConfig &tun1, const SSHConnectionConfig &tun2);
    friend bool operator!=(const SSHConnectionConfig &tun1, const SSHConnectionConfig &tun2);
  };

  enum class SSHFingerprint {
    STORE,
    REJECT
  };

  enum class SSHReturnType {
    CONNECTION_FAILURE,
    CONNECTED,
    INVALID_AUTH_DATA,
    FINGERPRINT_MISMATCH,
    FINGERPRINT_CHANGED,
    FINGERPRINT_UNKNOWN_AUTH_FILE_MISSING,
    FINGERPRINT_UNKNOWN
  };
  enum class SSHAuthtype {
    PASSWORD,
    KEYFILE,
    AUTOPUBKEY
  };

  class SSHConnectionCredentials {
  public:
    std::string username;
    std::string password;
    std::string keyfile;
    std::string keypassword;
    SSHFingerprint fingerprint;
    SSHAuthtype auth;
  };

  class SSHTunnelException : public std::exception {
  public:
    explicit SSHTunnelException(const std::string &message)
        : _msgText(message) {
    }
    explicit SSHTunnelException(const char *message)
        : _msgText(message) {
    }
    virtual ~SSHTunnelException() NOEXCEPT {
    }
    virtual const char *what() const NOEXCEPT {
      return _msgText.c_str();
    }
  protected:
    std::string _msgText;
  };

  class SSHSftpException : public std::exception {
  public:
    explicit SSHSftpException(const std::string &message)
        : _msgText(message) {
    }
    explicit SSHSftpException(const char *message)
        : _msgText(message) {
    }
    virtual ~SSHSftpException() NOEXCEPT {
    }
    virtual const char *what() const NOEXCEPT {
      return _msgText.c_str();
    }
  protected:
    std::string _msgText;
  };

  class SSHAuthException : public std::exception {
  public:
    explicit SSHAuthException(const std::string &message)
        : _msgText(message) {
    }
    explicit SSHAuthException(const char *message)
        : _msgText(message) {
    }
    virtual ~SSHAuthException() NOEXCEPT {
    }
    virtual const char *what() const NOEXCEPT {
      return _msgText.c_str();
    }
  protected:
    std::string _msgText;
  };

  class SSHThread {
  public:
    SSHThread();
    virtual ~SSHThread();
    virtual void stop();
    bool isRunning();
    void start();
    void join();

  protected:
    std::atomic<bool> _stop;
    std::atomic<bool> _finished;
    base::Semaphore _initializationSem;
    virtual void run() = 0;
    void _run();

  private:
    std::thread _thread;
  };

} /* namespace ssh */
