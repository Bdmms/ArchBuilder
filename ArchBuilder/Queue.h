#pragma once

#ifndef CYCLE_QUEUE_H
#define CYCLE_QUEUE_H

/**
 * @brief Fixed cyclical queue
 * @tparam T - storage type
*/
template <typename T, unsigned int Size>
struct CycleQueue
{
	T buffer[Size];
	unsigned int start = 0;
	unsigned int end = 0;

	/**
	 * @brief Creates an empty queue
	*/
	constexpr CycleQueue() : buffer() {}

	/**
	 * @brief Adds the value to the back of the queue.
	 * @param value - queued value
	*/
	constexpr void queue(const T value)
	{
		buffer[end] = value;
		end = (end + 1) % Size;
	}

	/**
	 * @brief Removes the value from the front of the queue.
	 * @return removed queued value
	*/
	constexpr T dequeue()
	{
		T& value = buffer[start];
		start = (start + 1) % Size;
		return value;
	}
};

#endif