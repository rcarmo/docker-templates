FROM rcarmo/desktop-chrome
MAINTAINER rcarmo
# loosely based on https://github.com/samtstern/android-vagrant and https://github.com/dockerfile/java/blob/master/oracle-java8

ENV DEBIAN_FRONTEND noninteractive

# Install Java.
RUN \
  echo oracle-java8-installer shared/accepted-oracle-license-v1-1 select true | debconf-set-selections && \
  add-apt-repository -y ppa:webupd8team/java && \
  apt-get update && \
  apt-get install -y oracle-java8-installer && \
  rm -rf /var/lib/apt/lists/* && \
  rm -rf /var/cache/oracle-jdk8-installer

ENV JAVA_HOME /usr/lib/jvm/java-8-oracle

# Dependencies
RUN dpkg --add-architecture i386 && apt-get update && apt-get install -yq curl libstdc++6:i386 zlib1g:i386 libncurses5:i386 ant maven --no-install-recommends
ENV GRADLE_URL http://services.gradle.org/distributions/gradle-2.3-all.zip
RUN curl -L ${GRADLE_URL} -o /tmp/gradle-2.3-all.zip && unzip /tmp/gradle-2.3-all.zip -d /usr/local && rm /tmp/gradle-2.3-all.zip
ENV GRADLE_HOME /usr/local/gradle-2.3

# Download and untar SDK
ENV ANDROID_SDK_URL http://dl.google.com/android/android-sdk_r24.4.1-linux.tgz
RUN curl -L ${ANDROID_SDK_URL} | tar xz -C /usr/local
ENV ANDROID_HOME /usr/local/android-sdk-linux

# Install Android SDK components
ENV ANDROID_SDK_COMPONENTS platform-tools,build-tools-21.1.2,android-21,android-17,extra-android-support
RUN echo y | ${ANDROID_HOME}/tools/android update sdk --no-ui --all --filter "${ANDROID_SDK_COMPONENTS}"

# Path
ENV PATH $PATH:${ANDROID_HOME}/tools:$ANDROID_HOME/platform-tools:${GRADLE_HOME}/bin

# Download and Unzip Android Studio
ENV ANDROID_STUDIO_URL https://dl.google.com/dl/android/studio/ide-zips/1.4.1.0/android-studio-ide-141.2343393-linux.zip
RUN curl -L ${ANDROID_STUDIO_URL} -o /tmp/android-studio-ide.zip && unzip /tmp/android-studio-ide.zip -d /usr/local && rm /tmp/android-studio-ide.zip
ENV ANDROID_STUDIO_HOME /usr/local/android-studio

# Install extra Android SDK
ENV ANDROID_SDK_EXTRA_COMPONENTS extra-google-google_play_services,extra-google-m2repository,extra-android-m2repository,source-21,addon-google_apis-google-21,sys-img-x86-addon-google_apis-google-21
RUN echo y | ${ANDROID_HOME}/tools/android update sdk --no-ui --all --filter "${ANDROID_SDK_EXTRA_COMPONENTS}"

# Path
ENV PATH $PATH:${ANDROID_STUDIO_HOME}/bin
