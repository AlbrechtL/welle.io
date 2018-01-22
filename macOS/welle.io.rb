class WelleIo < Formula
  desc "DAB/DAB+ Software Radio"
  homepage "https://www.welle.io"
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
