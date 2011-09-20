AM_CXXFLAGS = -ggdb
#CXXFLAGS = -O0

bin_PROGRAMS = torproxy torscanner

torproxy_SOURCES = TorProxy.cpp TorProxy.h ShuffleStream.h protocol/RelayCellDispatcher.h protocol/HybridEncryption.h protocol/HybridEncryption.cpp protocol/Connection.cpp protocol/Connection.h protocol/Cell.cpp protocol/Cell.h protocol/Directory.cpp protocol/Directory.h protocol/RelayDataCell.h protocol/CreatedCell.h protocol/RelayCell.h protocol/RelayEndCell.h protocol/RelaySendMeCell.h protocol/ServerListing.cpp protocol/ServerListing.h util/Util.cpp protocol/Circuit.cpp protocol/Circuit.h protocol/CellEncrypter.cpp protocol/CellEncrypter.h protocol/RelayBeginCell.h protocol/CellListener.h protocol/RelayCellDispatcher.cpp protocol/CellConsumer.cpp protocol/CellConsumer.h ProxyShuffler.cpp protocol/CreateCell.cpp protocol/CreateCell.h protocol/CreatedCell.cpp TorTunnel.cpp TorTunnel.h SocksConnection.cpp SocksConnection.h util/Network.cpp ProxyShuffler.h util/Network.h util/Util.h


torproxy_LDFLAGS = -lssl -lboost_system-mt -lcrypto

torscanner_SOURCES = TorScanner.cpp TorScanner.h ShuffleStream.h protocol/RelayCellDispatcher.h protocol/HybridEncryption.h protocol/HybridEncryption.cpp protocol/Connection.cpp protocol/Connection.h protocol/Cell.cpp protocol/Cell.h protocol/Directory.cpp protocol/Directory.h protocol/RelayDataCell.h protocol/CreatedCell.h protocol/RelayCell.h protocol/RelayEndCell.h protocol/RelaySendMeCell.h protocol/ServerListing.cpp protocol/ServerListing.h util/Util.cpp protocol/Circuit.cpp protocol/Circuit.h protocol/CellEncrypter.cpp protocol/CellEncrypter.h protocol/RelayBeginCell.h protocol/CellListener.h protocol/RelayCellDispatcher.cpp protocol/CellConsumer.cpp protocol/CellConsumer.h ProxyShuffler.cpp protocol/CreateCell.cpp protocol/CreateCell.h protocol/CreatedCell.cpp TorTunnel.cpp TorTunnel.h util/Network.cpp protocol/ServerListingGroup.cpp protocol/ServerListingGroup.h util/Network.h util/Util.h

torscanner_LDFLAGS = -lssl -lboost_system-mt -lcrypto
