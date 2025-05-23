#ifndef COWEL_DIAGNOSTIC_POLICY_HPP
#define COWEL_DIAGNOSTIC_POLICY_HPP

#include "cowel/fwd.hpp"

#include "compilation_stage.hpp"

namespace cowel {

enum struct Policy_Action : Default_Underlying {
    /// @brief Immediate success.
    success,
    /// @brief Immediate failure.
    failure,
    /// @brief Keep going.
    keep_going
};

[[nodiscard]]
constexpr bool is_exit(Policy_Action action)
{
    return action != Policy_Action::keep_going;
}

struct Diagnostic_Policy {
    [[nodiscard]]
    virtual bool is_success() const
        = 0;

    virtual Policy_Action error(IO_Error_Code) = 0;

    virtual Policy_Action done(Compilation_Stage) = 0;
};

} // namespace cowel

#endif
