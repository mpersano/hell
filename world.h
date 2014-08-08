#pragma once

#include <memory>

class world_impl;

class world
{
public:
	world(int width, int height);
	~world();

	void draw() const;
	void update();

private:
	std::unique_ptr<world_impl> impl_;

	world(const world&) = delete;
	world& operator=(const world&) = delete;
};
