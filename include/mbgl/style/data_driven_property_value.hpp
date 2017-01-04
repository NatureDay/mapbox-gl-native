#pragma once

#include <mbgl/util/variant.hpp>
#include <mbgl/style/undefined.hpp>
#include <mbgl/style/function/camera_function.hpp>
#include <mbgl/style/function/source_function.hpp>
#include <mbgl/style/function/composite_function.hpp>

namespace mbgl {
namespace style {

template <class T>
class DataDrivenPropertyValue {
private:
    using Value = variant<
        Undefined,
        T,
        CameraFunction<T>,
        SourceFunction<T>,
        CompositeFunction<T>>;

    Value value;

    friend bool operator==(const DataDrivenPropertyValue& lhs,
                           const DataDrivenPropertyValue& rhs) {
        return lhs.value == rhs.value;
    }

public:
    DataDrivenPropertyValue() = default;
    DataDrivenPropertyValue(                  T  v) : value(std::move(v)) {}
    DataDrivenPropertyValue(   CameraFunction<T> v) : value(std::move(v)) {}
    DataDrivenPropertyValue(   SourceFunction<T> v) : value(std::move(v)) {}
    DataDrivenPropertyValue(CompositeFunction<T> v) : value(std::move(v)) {}

    bool isUndefined()          const { return value.template is<Undefined>(); }
    bool isConstant()           const { return value.template is<T>(); }
    bool isCameraFunction()     const { return value.template is<CameraFunction<T>>(); }
    bool isSourceFunction()     const { return value.template is<SourceFunction<T>>(); }
    bool isCompositeFunction()  const { return value.template is<CompositeFunction<T>>(); }

    const                    T &  asConstant()          const { return value.template get<T>(); }
    const CameraFunction    <T>&  asCameraFunction()    const { return value.template get<CameraFunction<T>>(); }
    const SourceFunction    <T>&  asSourceFunction()    const { return value.template get<SourceFunction<T>>(); }
    const CompositeFunction <T>&  asCompositeFunction() const { return value.template get<CompositeFunction<T>>(); }

    template <typename Evaluator>
    auto evaluate(const Evaluator& evaluator) const {
        return Value::visit(value, evaluator);
    }
};

} // namespace style
} // namespace mbgl
