#pragma once

#define Assert(x) do {if (!(x)) __debugbreak(); } while (0)
#define Unimplemented() Assert(false)
