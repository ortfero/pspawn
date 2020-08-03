/* This file is part of pspawn library
 * Copyright 2020 Andrei Ilin <ortfero@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once


#include <cstdint>
#include <system_error>
#include <chrono>
#include <optional>


#ifdef _WIN32

#include <cstring>

#if !defined(_X86_) && !defined(_AMD64_) && !defined(_ARM_) && !defined(_ARM64_)
#if defined(_M_IX86)
#define _X86_
#elif defined(_M_AMD64)
#define _AMD64_
#elif defined(_M_ARM)
#define _ARM_
#elif defined(_M_ARM64)
#define _ARM64_
#endif
#endif

#include <minwindef.h>
#include <errhandlingapi.h>
#include <handleapi.h>
#include <synchapi.h>
#include <processthreadsapi.h>

#else
  
#error Unsupported system

#endif // WIN32


namespace pspawn {
  
  enum class priority {
    low, normal, high
  };
  
  
  enum class process_status {
    deffered, running, done
  };
  
    
  class process {
  public:
  
    static std::error_code last_error() noexcept {
      return std::error_code{int(GetLastError()), std::system_category()};
    }
  
  
    static process spawn(std::string command,
                         enum priority priority = priority::normal,
                         std::string directory = std::string{},
                         std::chrono::milliseconds terminate_timeout = std::chrono::milliseconds{ 3000 }) noexcept {
      constexpr auto create_new_console = 0x00000010u;
      process result;
      result.terminate_timeout_ = terminate_timeout;
      STARTUPINFOA si;
      std::memset(&si, 0, sizeof(si));
      si.cb = sizeof(si);
      DWORD creation_flags = create_new_console;
      switch(priority) {
      case priority::low:
        creation_flags |= 0x00000040; break;
      case priority::high:
        creation_flags |= 0x00000080; break;
      default:
        creation_flags |= 0x00000020; break;      
      }
      char const* work_dir = directory.empty() ? nullptr : directory.data();
      if(!CreateProcessA(nullptr, command.data(), nullptr, nullptr, FALSE,
                         creation_flags, nullptr, work_dir, &si, &result.pi_))
        return std::move(result);
      return std::move(result);
    }    
  
    
    process() noexcept { pi_.hProcess = nullptr; }
    
    process(process const&) = delete;
    process& operator = (process const&) = delete;
    
    
    process(process&& other) noexcept:
      terminate_timeout_{other.terminate_timeout_} {
      std::memcpy(&pi_, &other.pi_, sizeof(PROCESS_INFORMATION)); other.pi_.hProcess = nullptr;
    }
    
    
    process& operator = (process&& other) noexcept {
      cleanup();
      std::memcpy(&pi_, &other.pi_, sizeof(PROCESS_INFORMATION)); other.pi_.hProcess = nullptr;
      terminate_timeout_ = other.terminate_timeout_;
      return *this;
    }
    
    
    ~process() {
      cleanup();
    }
    
    
    explicit operator bool () const noexcept {
      return pi_.hProcess != nullptr;
    }
    
    
    bool done() const noexcept {
      if(pi_.hProcess == nullptr)
        return false;
      return WaitForSingleObject(pi_.hProcess, 0) != WAIT_TIMEOUT;
    }
    
    
    void wait() noexcept {
      if(pi_.hProcess == nullptr)
        return;
      WaitForSingleObject(pi_.hProcess, INFINITE);
    }
    
    
    template<class Rep, class Period>
    process_status wait_for(std::chrono::duration<Rep, Period> const& timeout) noexcept {
      if(pi_.hProcess == nullptr)
        return process_status::deffered;
      auto const ms = std::chrono::duration_cast<std::chrono::milliseconds>(timeout);
      if(WaitForSingleObject(pi_.hProcess, ms.count()) == WAIT_TIMEOUT)
        return process_status::running;
      return process_status::done;
    }
    
    
    std::optional<std::uint32_t> exit_code() const noexcept {
      std::optional<std::uint32_t> result;
      if(pi_.hProcess == nullptr)
        return result;
      DWORD exit_code;
      GetExitCodeProcess(pi_.hProcess, &exit_code);
      return result = exit_code;
    }
    
    
  private:
  
    PROCESS_INFORMATION pi_;
    std::chrono::milliseconds terminate_timeout_;
    
    void cleanup() {
      if(pi_.hProcess == nullptr)
        return;
      if(WaitForSingleObject(pi_.hProcess, static_cast<DWORD>(terminate_timeout_.count())) == WAIT_TIMEOUT)
        TerminateProcess(pi_.hProcess, -1);
      CloseHandle(pi_.hProcess);
      CloseHandle(pi_.hThread);
    }
    
  };
  
  
}
