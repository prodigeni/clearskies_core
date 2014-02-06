/*
 *  This file is part of clearskies_core file synchronization program
 *  Copyright (C) 2014 Pedro Larroy

 *  clearskies_core is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  clearskies_core is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.

 *  You should have received a copy of the GNU Lesser General Public License
 *  along with clearskies_core.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include "int_types.h"
#include <array>
#include <string>
#include <thread>
#include "sqlite3pp/sqlite3pp.h"

namespace cs
{
namespace share
{

class Share
{
public:
    Share(const std::string& share_path, const std::string& dbpath = ":memory:");


    void scan();

    void scan_thread();

private:
    std::thread m_scan_thread;

public:
    std::string m_path;
    sqlite3pp::database m_db;
    std::string m_db_path;
    
    /// share id, shared publicly
    std::array<u8, 32> m_share_id; 
    std::array<u8, 16> m_peer_id; 
    /// pre-shared key read-write 
    std::array<u8, 16> m_psk_rw; 
    /// pre-shared key read-only 
    std::array<u8, 16> m_psk_ro; 
    /// pre-shared key untrusted
    std::array<u8, 16> m_psk_ut; 
    /// pre-shared key untrusted

    /// private keys 
    std::array<u8, 256> m_pkc_rw;
    std::array<u8, 256> m_pkc_ro;
};



} // end ns
} // end ns