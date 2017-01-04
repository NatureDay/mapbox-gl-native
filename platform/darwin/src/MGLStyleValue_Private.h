#import <Foundation/Foundation.h>

#import "MGLStyleValue.h"

#import "NSValue+MGLStyleAttributeAdditions.h"
#import "MGLTypes.h"

#import <mbgl/util/enum.hpp>

#include <mbgl/style/data_driven_property_value.hpp>

#if TARGET_OS_IPHONE
    #import "UIColor+MGLAdditions.h"
#else
    #import "NSColor+MGLAdditions.h"
#endif

template <typename MBGLType, typename ObjCType, typename MBGLElement = MBGLType, typename ObjCEnum = ObjCType>
class MGLStyleValueTransformer {
public:

    // MBGL -> MGL
    
    MGLStyleValue<ObjCType> *toStyleValue(const mbgl::style::PropertyValue<MBGLType> &mbglValue) {
        if (mbglValue.isConstant()) {
            //return toStyleConstantValue(mbglValue.asConstant());
        }
        // TODO: Translate from function variants
//        else if (mbglValue.isFunction()) {
//            return toStyleFunction(mbglValue.asFunction());
//        }
        else {
            return nil;
        }
    }

    MGLStyleValue<ObjCType> *toDataDrivenStyleValue(const mbgl::style::DataDrivenPropertyValue<MBGLType> &mbglValue) {
        if (mbglValue.isConstant()) {
            return toStyleConstantValue(mbglValue.asConstant());
        } else if (mbglValue.isSourceFunction()) {
            auto sourceFunction = mbglValue.asSourceFunction();
            if (sourceFunction.hasExponentialStops()) {
                return toStyleSourceExponentialFunction(sourceFunction);
            } else if (sourceFunction.hasIntervalStops()) {
                return toStyleSourceIntervalFunction(sourceFunction);
            } else if (sourceFunction.hasCategoricalStops()) {
                return toStyleSourceCategoricalFunction(sourceFunction);
            } else if (sourceFunction.hasIdentityStops()) {
                return toStyleSourceIdentityFunction(sourceFunction);
            }
            return nil;
        }
        return [MGLStyleSourceExponentialFunction functionWithAttributeName:@"" exponentialStops:@{}];
    }

    template <typename MBGLEnum = MBGLType,
              class = typename std::enable_if<std::is_enum<MBGLEnum>::value>::type,
              typename MGLEnum = ObjCEnum,
              class = typename std::enable_if<std::is_enum<MGLEnum>::value>::type>
    MGLStyleValue<ObjCType> *toEnumStyleValue(const mbgl::style::PropertyValue<MBGLEnum> &mbglValue) {
        if (mbglValue.isConstant()) {
//            return toEnumStyleConstantValue<>(mbglValue.asConstant());
            return nil;
        }
        // TODO: Translate from function variants
//        else if (mbglValue.isFunction()) {
//            const auto &mbglStops = mbglValue.asFunction().getStops();
//            NSMutableDictionary *stops = [NSMutableDictionary dictionaryWithCapacity:mbglStops.size()];
//            for (const auto &mbglStop : mbglStops) {
//                stops[@(mbglStop.first)] = toEnumStyleConstantValue<>(mbglStop.second);
//            }
//            return [MGLStyleFunction<NSValue *> functionWithBase:mbglValue.asFunction().getBase() stops:stops];
//        }
        else {
            return nil;
        }
    }

#pragma mark: MGL -> MBGL

    mbgl::style::PropertyValue<MBGLType> toPropertyValue(MGLStyleValue<ObjCType> *value) {
        if ([value isKindOfClass:[MGLStyleConstantValue class]]) {
            MBGLType mbglValue;
            id mglValue = [(MGLStyleConstantValue<ObjCType> *)value rawValue];
            getMBGLValue(mglValue, mbglValue);
            return mbglValue;
        } else if (value) {
            [NSException raise:@"MGLAbstractClassException" format:
             @"The style value %@ cannot be applied to the style. "
             @"Make sure the style value was created as a member of a concrete subclass of MGLStyleValue.",
             NSStringFromClass([value class])];
            return {};
        } else {
            return {};
        }
    }

    mbgl::style::DataDrivenPropertyValue<MBGLType> toDataDrivenPropertyValue(MGLStyleValue<ObjCType> *value) {
        if ([value isKindOfClass:[MGLStyleConstantValue class]]) {
            MBGLType mbglValue;
            id mglValue = [(MGLStyleConstantValue<ObjCType> *)value rawValue];
            getMBGLValue(mglValue, mbglValue);
            return mbglValue;
        } else if ([value isKindOfClass:[MGLStyleFunction class]]) {
            MGLStyleFunction<ObjCType> *function = (MGLStyleFunction<ObjCType> *)value;

            __block std::map<float, MBGLType> stops = {};
            [function.stops enumerateKeysAndObjectsUsingBlock:^(NSNumber * _Nonnull zoomKey, MGLStyleValue<ObjCType> * _Nonnull stopValue, BOOL * _Nonnull stop) {
                NSCAssert([stopValue isKindOfClass:[MGLStyleValue class]], @"Stops should be MGLStyleValues");
                auto mbglStopValue = toPropertyValue(stopValue);
                NSCAssert(mbglStopValue.isConstant(), @"Stops must be constant");
                stops[zoomKey.floatValue] = mbglStopValue.asConstant();
            }];

            // Camera function with Exponential stops
            mbgl::style::ExponentialStops<MBGLType> exponentialStops = {stops, (float)function.base};
            mbgl::style::CameraFunction<MBGLType> cameraFunction = {exponentialStops};

            return cameraFunction;
        } else if ([value isKindOfClass:[MGLStyleIntervalFunction class]]) {
            MGLStyleIntervalFunction<ObjCType> *function = (MGLStyleIntervalFunction<ObjCType> *)value;

            __block std::map<float, MBGLType> stops = {};
            [function.stops enumerateKeysAndObjectsUsingBlock:^(NSNumber * _Nonnull zoomKey, MGLStyleValue<ObjCType> * _Nonnull stopValue, BOOL * _Nonnull stop) {
                NSCAssert([stopValue isKindOfClass:[MGLStyleValue class]], @"Stops should be MGLStyleValues");
                auto mbglStopValue = toPropertyValue(stopValue);
                NSCAssert(mbglStopValue.isConstant(), @"Stops must be constant");
                stops[zoomKey.floatValue] = mbglStopValue.asConstant();
            }];

            // Camera function with Interval stops
            mbgl::style::IntervalStops<MBGLType> intervalStops = {stops};
            mbgl::style::CameraFunction<MBGLType> cameraFunction = {intervalStops};

            return cameraFunction;
        } else if ([value isKindOfClass:[MGLStyleSourceCategoricalFunction class]]) {
            MGLStyleSourceCategoricalFunction<ObjCType> *function = (MGLStyleSourceCategoricalFunction<ObjCType> *)value;

            __block std::map<mbgl::style::CategoricalValue, MBGLType> stops = {};

            [function.stops enumerateKeysAndObjectsUsingBlock:^(id categoryKey, MGLStyleValue<ObjCType> *stopValue, BOOL *stop) {
                NSCAssert([stopValue isKindOfClass:[MGLStyleValue class]], @"Stops should be MGLStyleValues");
                auto mbglStopValue = toPropertyValue(stopValue);
                NSCAssert(mbglStopValue.isConstant(), @"Stops must be constant");

                if ([categoryKey isKindOfClass:[NSString class]]) {
                    const std::string& convertedValueKey = [((NSString *)categoryKey) UTF8String];
                    stops[mbgl::style::CategoricalValue(convertedValueKey)] = mbglStopValue.asConstant();
                } else if ([categoryKey isKindOfClass:[NSNumber class]]) {
                    NSNumber *key = (NSNumber *)categoryKey;
                    if ((strcmp([key objCType], @encode(char)) == 0) ||
                        (strcmp([key objCType], @encode(BOOL)) == 0)) {
                        stops[mbgl::style::CategoricalValue((bool)[key boolValue])] = mbglStopValue.asConstant();
                    } else if (strcmp([key objCType], @encode(double)) == 0 ||
                               strcmp([key objCType], @encode(float)) == 0) {
                        NSCAssert(mbglStopValue.isConstant(), @"Categorical stop keys must be strings, booleans, or integers");
                    } else if ([key compare:@(0)] == NSOrderedDescending ||
                               [key compare:@(0)] == NSOrderedSame ||
                               [key compare:@(0)] == NSOrderedAscending) {
                        stops[mbgl::style::CategoricalValue((int64_t)[key integerValue])] = mbglStopValue.asConstant();
                    }
                }
            }];
            mbgl::style::CategoricalStops<MBGLType> categoricalStops = {stops};
            if (function.defaultValue) {
                NSCAssert([function.defaultValue isKindOfClass:[MGLStyleConstantValue class]], @"Default value must be constant");
                MBGLType mbglValue;
                id mglValue = [(MGLStyleConstantValue<ObjCType> *)function.defaultValue rawValue];
                getMBGLValue(mglValue, mbglValue);
                categoricalStops.defaultValue = mbglValue;
            }
            mbgl::style::SourceFunction<MBGLType> sourceFunction = {function.attributeName.UTF8String, categoricalStops};
            return sourceFunction;
        } else if ([value isKindOfClass:[MGLStyleSourceExponentialFunction class]]) {
            MGLStyleSourceExponentialFunction<ObjCType> *function = (MGLStyleSourceExponentialFunction<ObjCType> *)value;
            __block std::map<float, MBGLType> stops = {};
            [function.stops enumerateKeysAndObjectsUsingBlock:^(NSNumber * _Nonnull zoomKey, MGLStyleValue<ObjCType> * _Nonnull stopValue, BOOL * _Nonnull stop) {
                NSCAssert([stopValue isKindOfClass:[MGLStyleValue class]], @"Stops should be MGLStyleValues");
                auto mbglStopValue = toPropertyValue(stopValue);
                NSCAssert(mbglStopValue.isConstant(), @"Stops must be constant");
                stops[zoomKey.floatValue] = mbglStopValue.asConstant();
            }];
            mbgl::style::ExponentialStops<MBGLType> exponentialStops = {stops, (float)function.base};
            mbgl::style::SourceFunction<MBGLType> sourceFunction = {function.attributeName.UTF8String, exponentialStops};
            return sourceFunction;
        } else if ([value isKindOfClass:[MGLStyleSourceIntervalFunction class]]) {
            MGLStyleSourceIntervalFunction<ObjCType> *function = (MGLStyleSourceIntervalFunction<ObjCType> *)value;
            __block std::map<float, MBGLType> stops = {};
            [function.stops enumerateKeysAndObjectsUsingBlock:^(NSNumber * _Nonnull zoomKey, MGLStyleValue<ObjCType> * _Nonnull stopValue, BOOL * _Nonnull stop) {
                NSCAssert([stopValue isKindOfClass:[MGLStyleValue class]], @"Stops should be MGLStyleValues");
                auto mbglStopValue = toPropertyValue(stopValue);
                NSCAssert(mbglStopValue.isConstant(), @"Stops must be constant");
                stops[zoomKey.floatValue] = mbglStopValue.asConstant();
            }];
            mbgl::style::IntervalStops<MBGLType> intervalStops = {stops};
            mbgl::style::SourceFunction<MBGLType> sourceFunction = {function.attributeName.UTF8String, intervalStops};
            return sourceFunction;
        } else if ([value isKindOfClass:[MGLStyleSourceIdentityFunction class]]) {
            MGLStyleSourceIdentityFunction<ObjCType> *function = (MGLStyleSourceIdentityFunction<ObjCType> *)value;
            mbgl::style::IdentityStops<MBGLType> identityStops;
            mbgl::style::SourceFunction<MBGLType> sourceFunction = {function.attributeName.UTF8String, identityStops};
            return sourceFunction;
        } else if ([value isKindOfClass:[MGLStyleCompositeCategoricalFunction class]]) {
            MGLStyleCompositeCategoricalFunction<ObjCType> *function = (MGLStyleCompositeCategoricalFunction<ObjCType> *)value;
            __block std::map<float, mbgl::style::CategoricalStops<MBGLType>> compositeStops = {};
            [function.stops enumerateKeysAndObjectsUsingBlock:^(NSNumber * _Nonnull zoomKey, MGLStyleValue<ObjCType> * _Nonnull stopValue, BOOL * _Nonnull stop) {
                NSCAssert([stopValue isKindOfClass:[MGLStyleSourceCategoricalFunction class]], @"Stops should be MGLStyleSourceCategoricalFunction");

                MGLStyleSourceCategoricalFunction<ObjCType> *innerFunction = (MGLStyleSourceCategoricalFunction<ObjCType> *)stopValue;
                __block std::map<mbgl::style::CategoricalValue, MBGLType> stops = {};

                [innerFunction.stops enumerateKeysAndObjectsUsingBlock:^(id categoryKey, MGLStyleValue<ObjCType> *innerStopValue, BOOL *innerStop) {
                    NSCAssert([innerStopValue isKindOfClass:[MGLStyleValue class]], @"Stops should be MGLStyleValues");
                    auto mbglStopValue = toPropertyValue(innerStopValue);
                    NSCAssert(mbglStopValue.isConstant(), @"Stops must be constant");

                    if ([categoryKey isKindOfClass:[NSString class]]) {
                        const std::string& convertedValueKey = [((NSString *)categoryKey) UTF8String];
                        stops[mbgl::style::CategoricalValue(convertedValueKey)] = mbglStopValue.asConstant();
                    } else if ([categoryKey isKindOfClass:[NSNumber class]]) {
                        NSNumber *key = (NSNumber *)categoryKey;
                        if ((strcmp([key objCType], @encode(char)) == 0) ||
                            (strcmp([key objCType], @encode(BOOL)) == 0)) {
                            stops[mbgl::style::CategoricalValue((bool)[key boolValue])] = mbglStopValue.asConstant();
                        } else if (strcmp([key objCType], @encode(double)) == 0 ||
                                   strcmp([key objCType], @encode(float)) == 0) {
                            NSCAssert(mbglStopValue.isConstant(), @"Categorical stop keys must be strings, booleans, or integers");
                        } else if ([key compare:@(0)] == NSOrderedDescending ||
                                   [key compare:@(0)] == NSOrderedSame ||
                                   [key compare:@(0)] == NSOrderedAscending) {
                            stops[mbgl::style::CategoricalValue((int64_t)[key integerValue])] = mbglStopValue.asConstant();
                        }
                    }
                }];
                mbgl::style::CategoricalStops<MBGLType> categoricalStops = {stops};
                if (innerFunction.defaultValue) {
                    NSCAssert([innerFunction.defaultValue isKindOfClass:[MGLStyleConstantValue class]], @"Default value must be constant");
                    MBGLType mbglValue;
                    id mglValue = [(MGLStyleConstantValue<ObjCType> *)innerFunction.defaultValue rawValue];
                    getMBGLValue(mglValue, mbglValue);
                    categoricalStops.defaultValue = mbglValue;
                }
                compositeStops[zoomKey.floatValue] = categoricalStops;
            }];
            mbgl::style::CompositeFunction<MBGLType> compositeFunction = {function.attributeName.UTF8String, compositeStops};
            return compositeFunction;
        } else if (value) {
            [NSException raise:@"MGLAbstractClassException" format:
             @"The style value %@ cannot be applied to the style. "
             @"Make sure the style value was created as a member of a concrete subclass of MGLStyleValue.",
             NSStringFromClass([value class])];
            return {};
        } else {
            return {};
        }
    }

    template <typename MBGLEnum = MBGLType,
              class = typename std::enable_if<std::is_enum<MBGLEnum>::value>::type,
              typename MGLEnum = ObjCEnum,
              class = typename std::enable_if<std::is_enum<MGLEnum>::value>::type>
    mbgl::style::PropertyValue<MBGLEnum> toEnumPropertyValue(MGLStyleValue<ObjCType> *value) {
        if ([value isKindOfClass:[MGLStyleConstantValue class]]) {
            MBGLEnum mbglValue;
            getMBGLValue([(MGLStyleConstantValue<ObjCType> *)value rawValue], mbglValue);
            return mbglValue;
        } else if (value) {
            [NSException raise:@"MGLAbstractClassException" format:
             @"The style value %@ cannot be applied to the style. "
             @"Make sure the style value was created as a member of a concrete subclass of MGLStyleValue.",
             NSStringFromClass([value class])];
            return {};
        } else {
            return {};
        }
    }

private: // MBGL -> MGL

    MGLStyleConstantValue<ObjCType> *toStyleConstantValue(const MBGLType mbglValue) {
        auto rawValue = toMGLRawStyleValue(mbglValue);
        return [MGLStyleConstantValue<ObjCType> valueWithRawValue:rawValue];
    }

    MGLStyleSourceExponentialFunction<ObjCType> *toStyleSourceExponentialFunction(const mbgl::style::SourceFunction<MBGLType> &mbglFunction) {
        const auto &mbglStops = mbglFunction.getExponentialStops();
        NSMutableDictionary *stops = [NSMutableDictionary dictionaryWithCapacity:mbglStops.stops.size()];
        for (const auto &mbglStop : mbglStops.stops) {
            auto rawValue = toMGLRawStyleValue(mbglStop.second);
            stops[@(mbglStop.first)] = [MGLStyleValue valueWithRawValue:rawValue];
        }
        return [MGLStyleSourceExponentialFunction functionWithAttributeName:@(mbglFunction.property.c_str()) base:mbglStops.base exponentialStops:stops];
    }

    MGLStyleSourceIntervalFunction<ObjCType> *toStyleSourceIntervalFunction(const mbgl::style::SourceFunction<MBGLType> &mbglFunction) {
        const auto &mbglStops = mbglFunction.getIntervalStops();
        NSMutableDictionary *stops = [NSMutableDictionary dictionaryWithCapacity:mbglStops.stops.size()];
        for (const auto &mbglStop : mbglStops.stops) {
            auto rawValue = toMGLRawStyleValue(mbglStop.second);
            stops[@(mbglStop.first)] = [MGLStyleValue valueWithRawValue:rawValue];
        }
        return [MGLStyleSourceIntervalFunction functionWithAttributeName:@(mbglFunction.property.c_str()) intervalStops:stops];
    }

    MGLStyleSourceCategoricalFunction<ObjCType> *toStyleSourceCategoricalFunction(const mbgl::style::SourceFunction<MBGLType> &mbglFunction) {
        const auto &mbglStops = mbglFunction.getCategoricalStops();
        NSMutableDictionary *stops = [NSMutableDictionary dictionaryWithCapacity:mbglStops.stops.size()];
        for (const auto &mbglStop : mbglStops.stops) {
            auto categoricalValue = mbglStop.first;
            auto rawValue = toMGLRawStyleValue(mbglStop.second);
            id stopKey;
            if (categoricalValue.isBool()) {
                stopKey = toMGLRawStyleValue(categoricalValue.asBool());
            } else if (categoricalValue.isInteger()) {
                stopKey = toMGLRawStyleValue(categoricalValue.asInteger());
            } else if (categoricalValue.isString()) {
                stopKey = toMGLRawStyleValue(categoricalValue.asString());
            }
            stops[stopKey] = [MGLStyleValue valueWithRawValue:rawValue];
        }
        MGLStyleValue<ObjCType> *defaultValue = [MGLStyleValue valueWithRawValue:toMGLRawStyleValue(mbglStops.defaultValue)];
        return [MGLStyleSourceCategoricalFunction functionWithAttributeName:@(mbglFunction.property.c_str()) categoricalStops:stops defaultValue:defaultValue];
    }

    MGLStyleSourceIdentityFunction<ObjCType> *toStyleSourceIdentityFunction(const mbgl::style::SourceFunction<MBGLType> &mbglFunction) {
        return [MGLStyleSourceIdentityFunction functionWithAttributeName:@(mbglFunction.property.c_str())];
    }

//    MGLStyleFunction<ObjCType> *toStyleFunction(const mbgl::style::Function<MBGLType> &mbglFunction) {
//        const auto &mbglStops = mbglFunction.getStops();
//        NSMutableDictionary *stops = [NSMutableDictionary dictionaryWithCapacity:mbglStops.size()];
//        for (const auto &mbglStop : mbglStops) {
//            auto rawValue = toMGLRawStyleValue(mbglStop.second);
//            stops[@(mbglStop.first)] = [MGLStyleValue valueWithRawValue:rawValue];
//        }
//        return [MGLStyleFunction<ObjCType> functionWithBase:mbglFunction.getBase() stops:stops];
//    }

//    template <typename MBGLEnum = MBGLType,
//              class = typename std::enable_if<std::is_enum<MBGLEnum>::value>::type,
//              typename MGLEnum = ObjCEnum,
//              class = typename std::enable_if<std::is_enum<MGLEnum>::value>::type>
//    MGLStyleConstantValue<ObjCType> *toEnumStyleConstantValue(const MBGLEnum mbglValue) {
//        auto str = mbgl::Enum<MBGLEnum>::toString(mbglValue);
//        MGLEnum mglType = *mbgl::Enum<MGLEnum>::toEnum(str);
//        return [MGLStyleConstantValue<ObjCType> valueWithRawValue:[NSValue value:&mglType withObjCType:@encode(MGLEnum)]];
//    }

    NSNumber *toMGLRawStyleValue(const bool mbglStopValue) {
        return @(mbglStopValue);
    }
    
    NSNumber *toMGLRawStyleValue(const float mbglStopValue) {
        return @(mbglStopValue);
    }

    NSNumber *toMGLRawStyleValue(const int64_t mbglStopValue) {
        return @(mbglStopValue);
    }

    NSString *toMGLRawStyleValue(const std::string &mbglStopValue) {
        return @(mbglStopValue.c_str());
    }
    
    // Offsets
    NSValue *toMGLRawStyleValue(const std::array<float, 2> &mbglStopValue) {
        return [NSValue mgl_valueWithOffsetArray:mbglStopValue];
    }
    
    // Padding
    NSValue *toMGLRawStyleValue(const std::array<float, 4> &mbglStopValue) {
        return [NSValue mgl_valueWithPaddingArray:mbglStopValue];
    }
    
    MGLColor *toMGLRawStyleValue(const mbgl::Color mbglStopValue) {
        return [MGLColor mgl_colorWithColor:mbglStopValue];
    }
    
    ObjCType toMGLRawStyleValue(const std::vector<MBGLElement> &mbglStopValue) {
        NSMutableArray *array = [NSMutableArray arrayWithCapacity:mbglStopValue.size()];
        for (const auto &mbglElement: mbglStopValue) {
            [array addObject:toMGLRawStyleValue(mbglElement)];
        }
        return array;
    }

private: // MGL -> MBGL

    // Bool
    void getMBGLValue(NSNumber *rawValue, bool &mbglValue) {
        mbglValue = !!rawValue.boolValue;
    }

    // Float
    void getMBGLValue(NSNumber *rawValue, float &mbglValue) {
        mbglValue = rawValue.floatValue;
    }

    // String
    void getMBGLValue(NSString *rawValue, std::string &mbglValue) {
        mbglValue = rawValue.UTF8String;
    }
    
    // Offsets
    void getMBGLValue(NSValue *rawValue, std::array<float, 2> &mbglValue) {
        mbglValue = rawValue.mgl_offsetArrayValue;
    }
    
    // Padding
    void getMBGLValue(NSValue *rawValue, std::array<float, 4> &mbglValue) {
        mbglValue = rawValue.mgl_paddingArrayValue;
    }

    // Color
    void getMBGLValue(MGLColor *rawValue, mbgl::Color &mbglValue) {
        mbglValue = rawValue.mgl_color;
    }

    // Array
    void getMBGLValue(ObjCType rawValue, std::vector<MBGLElement> &mbglValue) {
        mbglValue.reserve(rawValue.count);
        for (id obj in rawValue) {
            MBGLElement mbglElement;
            getMBGLValue(obj, mbglElement);
            mbglValue.push_back(mbglElement);
        }
    }

    // Enumerations
    template <typename MBGLEnum = MBGLType,
    class = typename std::enable_if<std::is_enum<MBGLEnum>::value>::type,
    typename MGLEnum = ObjCEnum,
    class = typename std::enable_if<std::is_enum<MGLEnum>::value>::type>
    void getMBGLValue(ObjCType rawValue, MBGLEnum &mbglValue) {
        MGLEnum mglEnum;
        [rawValue getValue:&mglEnum];
        auto str = mbgl::Enum<MGLEnum>::toString(mglEnum);
        mbglValue = *mbgl::Enum<MBGLEnum>::toEnum(str);
    }
};
