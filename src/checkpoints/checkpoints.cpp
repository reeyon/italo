// Copyright (c) 2014-2018, The Monero And Italo Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "checkpoints.h"

#include "common/dns_utils.h"
#include "string_tools.h"
#include "storages/portable_storage_template_helper.h" // epee json include
#include "serialization/keyvalue_serialization.h"
#include <vector>

using namespace epee;

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "checkpoints"

namespace cryptonote
{
  /**
   * @brief struct for loading a checkpoint from json
   */
  struct t_hashline
  {
    uint64_t height; //!< the height of the checkpoint
    std::string hash; //!< the hash for the checkpoint
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(height)
          KV_SERIALIZE(hash)
        END_KV_SERIALIZE_MAP()
  };

  /**
   * @brief struct for loading many checkpoints from json
   */
  struct t_hash_json {
    std::vector<t_hashline> hashlines; //!< the checkpoint lines from the file
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(hashlines)
        END_KV_SERIALIZE_MAP()
  };

  //---------------------------------------------------------------------------
  checkpoints::checkpoints()
  {
  }
  //---------------------------------------------------------------------------
  bool checkpoints::add_checkpoint(uint64_t height, const std::string& hash_str)
  {
    crypto::hash h = crypto::null_hash;
    bool r = epee::string_tools::hex_to_pod(hash_str, h);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse checkpoint hash string into binary representation!");

    // return false if adding at a height we already have AND the hash is different
    if (m_points.count(height))
    {
      CHECK_AND_ASSERT_MES(h == m_points[height], false, "Checkpoint at given height already exists, and hash for new checkpoint was different!");
    }
    m_points[height] = h;
    return true;
  }
  //---------------------------------------------------------------------------
  bool checkpoints::is_in_checkpoint_zone(uint64_t height) const
  {
    return !m_points.empty() && (height <= (--m_points.end())->first);
  }
  //---------------------------------------------------------------------------
  bool checkpoints::check_block(uint64_t height, const crypto::hash& h, bool& is_a_checkpoint) const
  {
    auto it = m_points.find(height);
    is_a_checkpoint = it != m_points.end();
    if(!is_a_checkpoint)
      return true;

    if(it->second == h)
    {
      MINFO("CHECKPOINT PASSED FOR HEIGHT " << height << " " << h);
      return true;
    }else
    {
      MWARNING("CHECKPOINT FAILED FOR HEIGHT " << height << ". EXPECTED HASH: " << it->second << ", FETCHED HASH: " << h);
      return false;
    }
  }
  //---------------------------------------------------------------------------
  bool checkpoints::check_block(uint64_t height, const crypto::hash& h) const
  {
    bool ignored;
    return check_block(height, h, ignored);
  }
  //---------------------------------------------------------------------------
  //FIXME: is this the desired behavior?
  bool checkpoints::is_alternative_block_allowed(uint64_t blockchain_height, uint64_t block_height) const
  {
    if (0 == block_height)
      return false;

    auto it = m_points.upper_bound(blockchain_height);
    // Is blockchain_height before the first checkpoint?
    if (it == m_points.begin())
      return true;

    --it;
    uint64_t checkpoint_height = it->first;
    return checkpoint_height < block_height;
  }
  //---------------------------------------------------------------------------
  uint64_t checkpoints::get_max_height() const
  {
    std::map< uint64_t, crypto::hash >::const_iterator highest = 
        std::max_element( m_points.begin(), m_points.end(),
                         ( boost::bind(&std::map< uint64_t, crypto::hash >::value_type::first, _1) < 
                           boost::bind(&std::map< uint64_t, crypto::hash >::value_type::first, _2 ) ) );
    return highest->first;
  }
  //---------------------------------------------------------------------------
  const std::map<uint64_t, crypto::hash>& checkpoints::get_points() const
  {
    return m_points;
  }

  bool checkpoints::check_for_conflicts(const checkpoints& other) const
  {
    for (auto& pt : other.get_points())
    {
      if (m_points.count(pt.first))
      {
        CHECK_AND_ASSERT_MES(pt.second == m_points.at(pt.first), false, "Checkpoint at given height already exists, and hash for new checkpoint was different!");
      }
    }
    return true;
  }

  bool checkpoints::init_default_checkpoints(network_type nettype)
  {
    if (nettype == TESTNET)
    {
      return true;
    }
    if (nettype == STAGENET)
    {

      return true;
    }
    ADD_CHECKPOINT(1,     "851b5a2f42d7331c7caaaadfffcb9ffd74b5550e4511ba63eb1896ac4a52bf23");
    ADD_CHECKPOINT(10,    "9d425b4f22c06b1ba0cc745c668382956549f7c91575fcd7ed2b1671c1f756fe");
    ADD_CHECKPOINT(100,   "76a829f386450eee12c90adcfec1bdd9e66901678ed36488cfeeda16bd67c2af");
    ADD_CHECKPOINT(1000,  "327144eacf486f6acfbe638ea83172a5de97a45cfd24e2f97fa63b70bb545fab");
    ADD_CHECKPOINT(2000,  "5a5b35955132462c65605aea5b9e6939343629b8cbb165cb712127dbb2140faa");
    ADD_CHECKPOINT(2500,  "d53225967b2af50ff86e30ed5e98fc0f9fee03f10cd3cf3148cb81a300aa7d2e");
    ADD_CHECKPOINT(3000,  "f8076b940fd39ca51b34274f64510a8c41ac25848a84f9d3ab368b9d40926c96");
    ADD_CHECKPOINT(4000,  "9da93821bed76cfc4206cb468f230be2218ffb5f1aca47279e52eab2001a86b6");
    ADD_CHECKPOINT(4500,  "2b2295cbfb1ce2b56a00270eeb73cb619b5c2d5643088851c8398ef4d9bfeef8");
    ADD_CHECKPOINT(5000,  "15dd04efb175b0eb53505b8955539fc060e7b52ca0f896ddb8814fa4a93d7be7");
    ADD_CHECKPOINT(5700,  "ebdadcad6b25c34c7878ecf7e3af234f4b761797adfe2deeecb1b4fb4ddad6f4");
    ADD_CHECKPOINT(10000,  "8351d90f74ff78463ca80bb814e6fa8b4d918b4301a3ef4c1365be4b5d0972b5");
    ADD_CHECKPOINT(15000,  "e9e96917c02ddaa0277a010a763f2b3434a7478538d01a5afece5fa10546fb9c");
    ADD_CHECKPOINT(20000,  "237d453ba3940d7a83934527a3f4775368bc9d6c3ebe094485de37e0f80454ec");
    ADD_CHECKPOINT(30000,  "c688cc6e04a001800c1af60073409ec18be851b798b15962b89ba5b42660b8fa");
    ADD_CHECKPOINT(31500,  "48cec3d8af328d18cd198933de0d1bfb5ace456244e36d1af2673e67fa74ab0a");
    ADD_CHECKPOINT(51000,  "5d96a2aee1ce04225c3d80d5a784caa73c3711285e7a332c3aaeb2a746684f7f");
    ADD_CHECKPOINT(51600,  "7cb2405bcbfba74c2161538824e6ba0e2bd2c61d93e4762a60770b9b75d931d9");
    ADD_CHECKPOINT(60000,  "05d77890092dece603d99b3265823b4f44afd227b8d4963e99b892a67f19b664");
    ADD_CHECKPOINT(70000,  "af0f029b4cf45fad60b6e342ca93e73e2e1d0f0a1c760a17da8c7c0174664d75");
    ADD_CHECKPOINT(80000,  "d15c026f57e862d3c6b0f0c5fa4304d0e3dae948d7076dca3120c00ca922fea2");
    ADD_CHECKPOINT(90000,  "0d7fd5629f5e4289e24e5255f1c662c11bc559c5f2d0aab61c9573653604e491");
    ADD_CHECKPOINT(100000, "88d2e291f5651e6cc236f487e60ca7b375631b10e9647718786947efe004049a");
    ADD_CHECKPOINT(110000, "caa5bd21e857591b924f787a2d1b6ef3f51c865ff74f40433886dd8718bc705e");
    ADD_CHECKPOINT(120000, "984133a7e6715c8653af2ff570e413ed37a395926c3c980e2a13d3d5e5abb520");
    ADD_CHECKPOINT(130000, "272298b07908d8b2b37d10295755b95c1f84c48e47d84de186512e9298fc4963");
    ADD_CHECKPOINT(140000, "fa7cc6c37ccfe95c5deac1687f8f9e67ddcc36f8f69940d881989f8c2bd750cd");
    ADD_CHECKPOINT(150000, "5d1939d934eb3ef4b4823576af8f0de08d5335bfe4cde52e4a974d5d1d4e9435");
    ADD_CHECKPOINT(160000, "92d5da3c45807c6a174edf61873099377513d89260a6d64e1dbd91a9eec634fe");
    ADD_CHECKPOINT(200000, "36b4ab8c2ced68b2d0ff28e4c8e8b201448b61d9d036f32725103288cf77b85f");
    ADD_CHECKPOINT(250000, "f362181b52c46f2e9011728bdf3bb5249381d97ba9a776bd052af7e7bd2d4d01");
    ADD_CHECKPOINT(300000, "a381a4203f1cc22cfb8836a7f375975d2a9f284e0d83867f98c8eb58e81ce70d");
    ADD_CHECKPOINT(320000, "fc0e714c4c094fdc6f48cbc40800e18942c97f7515c4d0c13019797526ee36bf");
	
    return true;
  }

  bool checkpoints::load_checkpoints_from_json(const std::string &json_hashfile_fullpath)
  {
    boost::system::error_code errcode;
    if (! (boost::filesystem::exists(json_hashfile_fullpath, errcode)))
    {
      LOG_PRINT_L1("Blockchain checkpoints file not found");
      return true;
    }

    LOG_PRINT_L1("Adding checkpoints from blockchain hashfile");

    uint64_t prev_max_height = get_max_height();
    LOG_PRINT_L1("Hard-coded max checkpoint height is " << prev_max_height);
    t_hash_json hashes;
    if (!epee::serialization::load_t_from_json_file(hashes, json_hashfile_fullpath))
    {
      MERROR("Error loading checkpoints from " << json_hashfile_fullpath);
      return false;
    }
    for (std::vector<t_hashline>::const_iterator it = hashes.hashlines.begin(); it != hashes.hashlines.end(); )
    {
      uint64_t height;
      height = it->height;
      if (height <= prev_max_height) {
	LOG_PRINT_L1("ignoring checkpoint height " << height);
      } else {
	std::string blockhash = it->hash;
	LOG_PRINT_L1("Adding checkpoint height " << height << ", hash=" << blockhash);
	ADD_CHECKPOINT(height, blockhash);
      }
      ++it;
    }

    return true;
  }

  bool checkpoints::load_checkpoints_from_dns(network_type nettype)
  {
    std::vector<std::string> records;

    // All four ItaloPulse domains have DNSSEC on and valid
    static const std::vector<std::string> dns_urls = { 
	
    };

    static const std::vector<std::string> testnet_dns_urls = { 
	
    };

    static const std::vector<std::string> stagenet_dns_urls = { 
	
    };

    if (!tools::dns_utils::load_txt_records_from_dns(records, nettype == TESTNET ? testnet_dns_urls : nettype == STAGENET ? stagenet_dns_urls : dns_urls))
      return true; // why true ?

    for (const auto& record : records)
    {
      auto pos = record.find(":");
      if (pos != std::string::npos)
      {
        uint64_t height;
        crypto::hash hash;

        // parse the first part as uint64_t,
        // if this fails move on to the next record
        std::stringstream ss(record.substr(0, pos));
        if (!(ss >> height))
        {
    continue;
        }

        // parse the second part as crypto::hash,
        // if this fails move on to the next record
        std::string hashStr = record.substr(pos + 1);
        if (!epee::string_tools::hex_to_pod(hashStr, hash))
        {
    continue;
        }

        ADD_CHECKPOINT(height, hashStr);
      }
    }
    return true;
  }

  bool checkpoints::load_new_checkpoints(const std::string &json_hashfile_fullpath, network_type nettype, bool dns)
  {
    bool result;

    result = load_checkpoints_from_json(json_hashfile_fullpath);
    if (dns)
    {
      result &= load_checkpoints_from_dns(nettype);
    }

    return result;
  }
}
