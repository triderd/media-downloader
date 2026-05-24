# Chaquopy — keep Python modules
-keep class com.chaquo.python.** { *; }
-keep class java.** { *; }

# Kotlin serialization
-keepattributes *Annotation*, InnerClasses
-keep class kotlinx.serialization.** { *; }
-keepclassmembers class kotlinx.serialization.json.** { *** Companion; }
-keepclasseswithmembers class kotlinx.serialization.json.** { kotlinx.serialization.KSerializer serializer(...); }
-keep,includedescriptorclasses class com.mediadownloader.**$$serializer { *; }
-keepclassmembers class com.mediadownloader.** { *** Companion; }
-keepclasseswithmembers class com.mediadownloader.** { kotlinx.serialization.KSerializer serializer(...); }

# OkHttp
-keep class okhttp3.** { *; }
-dontwarn okhttp3.**
-dontwarn okio.**

# Coroutines
-keepnames class kotlinx.coroutines.internal.MainDispatcherFactory {}
-keepnames class kotlinx.coroutines.CoroutineExceptionHandler {}

# Compose
-keep class androidx.compose.** { *; }
