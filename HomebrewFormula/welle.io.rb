class WelleIo < Formula
  desc "DAB/DAB+ Software Radio"
  homepage "https://www.welle.io"
  url "https://github.com/AlbrechtL/welle.io/archive/V1.0.tar.gz"
  sha256 "669ae5d471f723c32622cbf6ee37b66c3aefd8e02d6334b55d1fb60b3c22a883"
  head "https://github.com/AlbrechtL/welle.io.git"

  depends_on "cmake" => :build
  depends_on "qt"
  depends_on "fftw"
  depends_on "faad2"
  depends_on "librtlsdr"
  depends_on "libusb"

  def install
    system "cmake", ".", "-DRTLSDR=TRUE", "-DBUILD_WELLE_CLI=OFF", *std_cmake_args
    system "make", "install"
  end

  test do
    system "#{bin}/welle-io", "-v"
  end
end
