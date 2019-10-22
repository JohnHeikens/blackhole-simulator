#pragma once
#include "gravitytile.h"
struct atom
{
	atom() {

	}
	atom(const vec2 pos, const vec2 speed) :pos(pos),speed(speed){}
	vec2 pos;
	vec2 speed;
	gravitytile* currenttile = NULL;
};