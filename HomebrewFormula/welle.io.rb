class WelleIo < Formula
  desc "DAB/DAB+ Software Radio"
  homepage "https://www.welle.io"
  url  "https://github.com/AlbrechtL/welle.io/archive/v2.0-beta1.tar.gz"
  sha256 "b81df164fcf74ec58629afa1e00911d63a1f8abdc875dd7dda3a04d975db83d4"
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
