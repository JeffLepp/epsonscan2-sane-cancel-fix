#include "SaneWrapper/backend.h"
#include "SaneWrapper/epsonscan.h"

#include <cassert>
#include <cstdlib>

namespace {

int cancel_calls;
int create_calls;
int transfer_calls;
int dispose_calls;
SDIOperationType last_operation;

SDIError fake_do_scan_job(SDIScannerDriver *, SDIOperationType operation)
{
  ++cancel_calls;
  last_operation = operation;
  return kSDIErrorNone;
}

SDIError fake_image_create(SDIImage **image)
{
  ++create_calls;
  *image = reinterpret_cast<SDIImage *>(0x1);
  return kSDIErrorNone;
}

SDIError fake_next_event(SDIScannerDriver *, SDITransferEventType *, SDIImage *, SDIError *)
{
  ++transfer_calls;
  return kSDIErrorNone;
}

SDIInt fake_image_dispose(SDIImage *)
{
  ++dispose_calls;
  return kSDIErrorNone;
}

struct Fixture
{
  Epson_Scanner *scanner;
  device *hardware;
  Supervisor *supervisor;

  Fixture()
  {
    scanner = static_cast<Epson_Scanner *>(std::calloc(1, sizeof(*scanner)));
    // Epson's create_epson_device() uses the same zero-allocation pattern.
    hardware = static_cast<device *>(std::calloc(1, sizeof(*hardware)));
    supervisor = new Supervisor();
    assert(scanner && hardware && supervisor);
    hardware->sv = supervisor;
    scanner->hw = hardware;
    supervisor->driver = reinterpret_cast<SDIScannerDriver *>(0x2);
    supervisor->SDIScannerDriver_DoScanJobPtr_ = fake_do_scan_job;
    supervisor->SDIImage_CreatePtr_ = fake_image_create;
    supervisor->SDIScannerDriver_GetNextTransferEventPtr_ = fake_next_event;
    supervisor->SDIImage_DisposePtr_ = fake_image_dispose;
  }

  ~Fixture()
  {
    delete supervisor;
    std::free(hardware);
    std::free(scanner);
  }
};

void reset_counters()
{
  cancel_calls = create_calls = transfer_calls = dispose_calls = 0;
}

} // namespace

int main()
{
  {
    reset_counters();
    Fixture fixture;
    fixture.scanner->scan_ready = true;
    fixture.scanner->image_count = 4;
    sane_epsonscan2_cancel(fixture.scanner);
    assert(fixture.scanner->cancel_requested);
    assert(!fixture.scanner->scan_ready);
    assert(fixture.scanner->image_count == 0);
    assert(cancel_calls == 1);
    assert(last_operation == kSDIOperationTypeCancel);

    sane_epsonscan2_cancel(fixture.scanner);
    assert(cancel_calls == 1);
  }

  {
    reset_counters();
    Fixture fixture;
    sane_epsonscan2_cancel(fixture.scanner);
    assert(!fixture.scanner->cancel_requested);
    assert(cancel_calls == 0);
  }

  {
    reset_counters();
    Fixture fixture;
    fixture.scanner->scan_ready = true;
    fixture.scanner->scan_complete = true;
    sane_epsonscan2_cancel(fixture.scanner);
    assert(cancel_calls == 0);
    assert(create_calls == 1);
    assert(transfer_calls == 1);
    assert(dispose_calls == 1);
  }

  sane_epsonscan2_cancel(nullptr);
}
