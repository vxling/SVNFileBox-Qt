#include "svncommand.h"

namespace SVNFileBox {

SvnCommandCategory commandCategory(SvnCommand cmd)
{
    switch (cmd) {
        // ReadOnly
        case SvnCommand::Info:               return SvnCommandCategory::ReadOnly;
        case SvnCommand::Status:             return SvnCommandCategory::ReadOnly;
        case SvnCommand::GetRevision:         return SvnCommandCategory::ReadOnly;
        case SvnCommand::GetHeadRevision:     return SvnCommandCategory::ReadOnly;
        case SvnCommand::GetConflictedFiles:  return SvnCommandCategory::ReadOnly;
        case SvnCommand::GetLastChangedTime:  return SvnCommandCategory::ReadOnly;
        case SvnCommand::IsVersioned:         return SvnCommandCategory::ReadOnly;
        case SvnCommand::IsValidWorkingCopy:   return SvnCommandCategory::ReadOnly;
        case SvnCommand::TestConnection:       return SvnCommandCategory::ReadOnly;
        case SvnCommand::GetServerUpdatePaths: return SvnCommandCategory::ReadOnly;

        // LocalWrite
        case SvnCommand::Add:       return SvnCommandCategory::LocalWrite;
        case SvnCommand::Delete:    return SvnCommandCategory::LocalWrite;
        case SvnCommand::Move:      return SvnCommandCategory::LocalWrite;
        case SvnCommand::Revert:    return SvnCommandCategory::LocalWrite;
        case SvnCommand::Resolve:    return SvnCommandCategory::LocalWrite;
        case SvnCommand::BreakLock: return SvnCommandCategory::LocalWrite;

        // HeavyWrite
        case SvnCommand::Commit:   return SvnCommandCategory::HeavyWrite;
        case SvnCommand::Update:   return SvnCommandCategory::HeavyWrite;
        case SvnCommand::Checkout:  return SvnCommandCategory::HeavyWrite;
    }
    return SvnCommandCategory::ReadOnly;
}

} // namespace SVNFileBox