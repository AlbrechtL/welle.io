class WelleIo < Formula
  desc "DAB/DAB+ Software Radio"
  homepage "https://www.welle.io"
  url  "https://github.com/AlbrechtL/welle.io/archive/v2.0.tar.gz"
  sha256 "abfe999b6788ae57dfaaebea5e1db912565d60cc287c9eec4636b0e10eab4f9d"
  head "https://github.com/AlbrechtL/welle.io.git"

  depends_on "cmake" => :build
  depends_on "qt"
  depends_on "fftw"
  depends_on "faad2"
  depends_on "mpg123"
  depends_on "librtlsdr"
  depends_on "pothosware/homebrew-pothos/soapysdr"
  depends_on "pothosware/homebrew-pothos/soapyuhd"
  depends_on "libusb"

  def install
    system "cmake", ".", "-DRTLSDR=TRUE", "-DSOAPYSDR=TRUE", "-DBUILD_WELLE_CLI=OFF", *std_cmake_args
    system "make", "install"
  end

  test do
    system "#{bin}/welle-io", "-v"
  end
end
