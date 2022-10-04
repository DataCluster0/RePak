#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <sysinfoapi.h>
#include <vector>
#include <cstdint>
#include <string>
#include <fstream>
#include <rapidcsv/rapidcsv.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

#include <math/MathHelper.h>
#include <math/Half.h>
#include <math/Matrix.h>
#include <math/Quaternion.h>
#include <math/Vector2.h>
#include <math/Vector3.h>

using namespace Math;

#include "rmem.h"
#include "rpak.h"
#include "rtech.h"

#include "BinaryIO.h"
#include "RePak.h"
#include "Utils.h"
