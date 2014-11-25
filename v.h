#pragma once

#define Assert(x) do {if (!(x)) __debugbreak(); } while (0)
#define Unimplemented() Assert(false)

#define V_ERROR(msg) do { printf(msg); return 0; } while (0);

#define V_REQUIRE(x, error) \
do { \
	int r = (x); \
	if (!r) { \
		printf("ERROR: Required a " error ".\n"); \
		return r; \
				} \
} while (0) \

