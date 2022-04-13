#pragma once
#include "Command.h"
#include <queue>


class CommandQueue
{
public:

	///Push function
	///
	///Pushes new commands into the command queue
	void						push(const Command& command);
	
	///Pop function
	/// @returns a temp empty command
	Command						pop();

	/// Removes the command from the command queue
	/// @returns a bool value. It will return true if the queue is empty, or else it will return false.
	bool						isEmpty() const;


private:
	std::queue<Command>			mQueue;
};

