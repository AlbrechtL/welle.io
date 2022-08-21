class WelleIo < Formula
  desc "DAB/DAB+ Software Radio"
  homepage "https://www.welle.io"
  url "https://github.com/AlbrechtL/welle.io/archive/v2.4.tar.gz"
  sha256 "7c2a2ff7b6e0780aee8a30a2beedfa831ce67683e1d076a73cebc897637d0202"
  license "GPL-2.0-or-later"
  head "https://github.com/AlbrechtL/welle.io.git"

  livecheck do
    url :stable
    strategy :github_latest
  end

  depends_on "cmake" => :build
  depends_on "faad2"
  depends_on "fftw"
  depends_on "librtlsdr"
  depends_on "libusb"
  depends_on "mpg123"
  depends_on "qt@5"

  def install
    system "cmake", ".", *std_cmake_args,
           "-DRTLSDR=1",
           "-DSOAPYSDR=0",
           "-DWITH_APP_BUNDLE=0", # Homebrew prefers a UNIX-like directory structure
           "-DBUILD_WELLE_CLI=1"
    system "make", "install"
  end

  test do
    system "#{bin}/welle-io", "-v"
  end
end
