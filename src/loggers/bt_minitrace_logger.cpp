
#include "behaviortree_cpp/loggers/bt_minitrace_logger.h"

namespace minitrace {
#include "minitrace/minitrace.h"
}

namespace BT
{
std::atomic<bool> MinitraceLogger::ref_count(false);

MinitraceLogger::MinitraceLogger(const Tree &tree, const char* filename_json)
  : StatusChangeLogger(tree.root_node   )
{
    bool expected = false;
    if (!ref_count.compare_exchange_strong(expected, true))
    {
        throw LogicError("Only one instance of StdCoutLogger shall be created");
    }

    minitrace::mtr_register_sigint_handler();
    minitrace::mtr_init(filename_json);
    this->enableTransitionToIdle(true);
}

MinitraceLogger::~MinitraceLogger()
{
    minitrace::mtr_flush();
    minitrace::mtr_shutdown();
	ref_count = false;
}

void MinitraceLogger::callback(Duration /*timestamp*/,
                               const TreeNode& node, NodeStatus prev_status,
                               NodeStatus status)
{
    using namespace minitrace;

    const bool statusCompleted = (status == NodeStatus::SUCCESS || status == NodeStatus::FAILURE);

    const std::string category = toStr(node.type());
    std::list<std::string>::const_iterator iter = std::find(m_categorys.cbegin(), m_categorys.cend(), category);
    if (iter == m_categorys.cend()) m_categorys.push_back(toStr(node.type()));
    iter = --m_categorys.cend();
    const char* name = node.name().c_str();

    if (prev_status == NodeStatus::IDLE && statusCompleted)
    {
        MTR_INSTANT(iter->c_str(), name);
    }
    else if (status == NodeStatus::RUNNING)
    {
        MTR_BEGIN(iter->c_str(), name);
    }
    else if (prev_status == NodeStatus::RUNNING && statusCompleted)
    {
        MTR_END(iter->c_str(), name);
    }
}

void MinitraceLogger::flush()
{
    minitrace::mtr_flush();
}
}   // end namespace
