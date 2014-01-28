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
#include "protocolstate.hpp"
#include "message.hpp"
#include <cstdlib>
#include <cassert>

using namespace std;

namespace cs
{
namespace protocol
{

namespace
{

inline bool has_signature(const char c)
{
    return c == '$' || c == '&';
}


inline bool has_payload(const char c)
{
    return c == '!' || c == '&';
}

pair<bool, size_t> to_base10(const char* b, const char * const e)
{
    pair<bool, size_t> res;
    assert(b <= e);
    res.first = true;
    while (b != e)
    {
        res.second *= 10;
        if (*b >= '0' && *b <= '9')
            res.second += *b - '0';
        else
        {
            res.first = false;
            return res;
        }
        ++b;
    }
    return res;
}

/// shift the unprocessed data in the input buffer left
void trim_buff(std::string& buf, std::string::const_iterator from)
{
    assert(from >= cbegin(buf));
    assert(from <= cend(buf));
    copy(from, cend(buf), begin(buf));
    buf.resize(distance(from, cend(buf)));
}

} // end anon ns

PayLoadFound find_payload(const std::string& buff)
{
    PayLoadFound result;
    const size_t newline_pos = buff.find('\n');
    if (buff.size() > 9 && newline_pos == string::npos) // 9 comes from 16 MB limit in ascii + \n
    {
        // ignore all the garbage we recieved
        result.size_nl_sz = buff.size();
        result.garbage = true;
        return result;
    }
    else if (newline_pos == string::npos)
        // wait for \n
        return result;

    result.size_nl_sz = newline_pos + 1;
    if (result.size_nl_sz > 9)
    {
        // got too much stuff before \n
        result.garbage = true;
        return result;
    }

    auto b10 = to_base10(&buff[0], &buff[newline_pos]);
    if (! b10.first)
    {
        // something not numeric in base10
        result.garbage = true;
        return result;
    }
    result.data_sz = b10.second;
    if (result.data_sz > 16777216)
    {
        // ignore a chunk which is too big
        result.data_sz = 0;
        result.garbage = true;
        return result;
    }
    result.found = true;
    return result;
}

size_t ProtocolState::s_input_buff_size = 4096;

void ProtocolState::input(const char* data, size_t len)
{
    m_input_buff.append(data, len);
    auto trim_start = cbegin(m_input_buff);
    while (true)
    {
        if (! m_read_payload)
        {
            MsgFound found = find_message(m_input_buff);
            if (found)
            {
                message::Message msg(found.json, has_payload(found.prefix), found.signature);
                handle_message(msg);
                if (msg.m_has_payload)
                    m_read_payload = true;
            }
            if (found.garbage)
                handle_msg_garbage(m_input_buff);

            trim_start = found.end;
            if (trim_start != cbegin(m_input_buff))
                trim_buff(m_input_buff, trim_start);
            else
                break;
        }
        else
        {
            if (! m_pl_found)
                // we are not waiting for finishing a chunk
                m_pl_found = find_payload(m_input_buff);

            if (m_pl_found && (m_input_buff.size() >= m_pl_found.total_size()))
            {
                if (m_pl_found.data_sz != 0)
                {
                    handle_payload(&m_input_buff[m_pl_found.size_nl_sz], m_pl_found.data_sz);
                }
                else
                {
                    handle_payload_end();
                    m_read_payload = false;
                }
                trim_buff(m_input_buff, cbegin(m_input_buff) + m_pl_found.total_size());
                m_pl_found.reset();
            }
            else if (m_pl_found.garbage)
            {
                handle_pl_garbage(m_input_buff);
                trim_buff(m_input_buff, cbegin(m_input_buff) + m_pl_found.total_size());
                m_read_payload = false;
                m_pl_found.reset();
            }
            else
                break;
        }
    }
}

MsgFound find_message(const std::string& buff)
{
    MsgFound result;
    result.end = buff.begin();
    const size_t newline1_pos = buff.find('\n');

    /// minimum message: {}\n
    if (newline1_pos == string::npos)
        return result;

    auto json_end = buff.begin() + newline1_pos;
    if (buff.size() < 3)
    {
        result.garbage = true;
        result.end = json_end + 1;
        return result;
    }

    auto buff_i = buff.begin();
    auto signature_end = buff.begin();
    auto signature_begin = buff.begin();
    if (*buff_i != '{')
    {
        if (*(buff_i + 1) != '{')
        {
            result.garbage = true;
            result.end = json_end + 1;
            return result;
        }
        result.prefix = *buff_i++;
        if (has_signature(result.prefix))
        {
            if (buff.size() > newline1_pos + 1)
            {
                const size_t newline2_pos = buff.find('\n', newline1_pos + 1);
                if (newline2_pos == string::npos)
                    return result;
                signature_begin = buff.begin() + newline1_pos + 1;
                signature_end = buff.begin() + newline2_pos;
                result.end = signature_end + 1;
            }
            else
                // accumulate more until we get also the signature
                return result;
        }
        else
            result.end = json_end + 1;
    }
    else
        result.end = json_end + 1;
    result.json.assign(buff_i, json_end);
    result.signature.assign(signature_begin, signature_end);
    result.found = true;
    return result;
}




} // end ns
} // end ns