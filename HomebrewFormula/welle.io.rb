class WelleIo < Formula
  desc "DAB/DAB+ Software Radio"
  homepage "https://www.welle.io"
  url "https://github.com/AlbrechtL/welle.io/archive/V1.0-rc2.tar.gz"
  sha256 "a82c0607a568be33aaa24604b8c7d6d4f2895703c7a1f2393a02365136bcd367"
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
