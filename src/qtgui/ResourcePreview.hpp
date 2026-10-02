#pragma once
#include "GmatBase.hpp"
#include <memory>

// OrbitView destruction emits ClearObjects under the object's original name.
// UI validation/serialization clones have no ownership of that live display.
// Suppress only their synchronous destruction callback, never engine cleanup.
namespace QtResourcePreviewDetail {
inline thread_local unsigned cleanupDepth=0;
struct CleanupScope {
   CleanupScope() { ++cleanupDepth; }
   ~CleanupScope() { --cleanupDepth; }
   CleanupScope(const CleanupScope &)=delete;
   CleanupScope &operator=(const CleanupScope &)=delete;
};
}
struct QtResourcePreviewDeleter {
   void operator()(GmatBase *value) const {
      QtResourcePreviewDetail::CleanupScope scope;
      delete value;
   }
};
using QtResourcePreview=std::unique_ptr<GmatBase,QtResourcePreviewDeleter>;
inline std::shared_ptr<GmatBase> qtResourcePreviewShared(GmatBase *value) {
   return {value,QtResourcePreviewDeleter{}};
}
inline std::shared_ptr<GmatBase> qtResourcePreviewShared(QtResourcePreview value) {
   return std::shared_ptr<GmatBase>(std::move(value));
}
inline bool qtResourcePreviewCleanupActive() {
   return QtResourcePreviewDetail::cleanupDepth!=0;
}
