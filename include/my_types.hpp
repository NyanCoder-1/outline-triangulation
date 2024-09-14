#pragma once

#include <functional>
#include <memory>

typedef std::shared_ptr<class Core> CorePtr;
typedef std::weak_ptr<class Core> CoreWeakPtr;
typedef std::shared_ptr<class Pipeline> PipelinePtr;
typedef std::shared_ptr<class Mesh> MeshPtr;

typedef std::function<bool(const CorePtr)> OnInitType;
typedef std::function<void(const CorePtr)> OnDestroyType;
typedef std::function<void(const CorePtr)> OnResizeType;
typedef std::function<bool(const CorePtr)> OnFrameType;

typedef bool ErrorFlag;

class MyDefer {
public:
	MyDefer(std::function<void()> func) : func(func) {}
	~MyDefer() { func(); }
private:
	std::function<void()> func;
};
