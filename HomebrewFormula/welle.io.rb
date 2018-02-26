class WelleIo < Formula
  desc "DAB/DAB+ Software Radio"
  homepage "https://www.welle.io"
  url "https://github.com/AlbrechtL/welle.io/archive/V1.0-rc3.tar.gz"
  sha256 "eb1d56c5d3442f1b5870866f0effb9022ba7426e51a966e5a7f7ac98e76b17f2"
  head "https://github.com/AlbrechtL/welle.io.git"

  depends_on "cmake" => :build
  depends_on "qt"
  depends_on "fftw"
  depends_on "faad2"
  depends_on "librtlsdr"
  depends_on "libusb"

  def install
    system "cmake", ".", "-DRTLSDR=TRUE", *std_cmake_args
    system "make", "install"
  end

  test do
    system "#{bin}/welle-io", "-v"
  end
end
