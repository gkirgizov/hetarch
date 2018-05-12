/* Copyright 2015 Yurii Litvinov and CyberTech Labs Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. */

#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace trikHal {
namespace trik {

class TrikMspI2c
{
public:
	TrikMspI2c() = default;
	~TrikMspI2c();

	void send(const std::vector<uint8_t> &data);
	int read(const std::vector<uint8_t> &data);
	bool connect(const std::string &devicePath, int deviceId);
	void disconnect();

private:
	/// Low-level descriptor of I2C device file.
	int mDeviceFileDescriptor = -1;
};

}
}
