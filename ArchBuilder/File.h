#pragma once

#ifndef ARC_FILE_H
#define ARC_FILE_H

// A readable file
class ARCFile
{
protected:
	bool valid = false;
	virtual bool load(const char*) = 0;
public:
	constexpr bool isValid() const { return valid; }
};

#endif