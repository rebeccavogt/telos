
#define DEFAULT_PRICE 500
#define FEE_PERCENTAGE 25

#define PRICE_MULTIPLIER 1.35
#define CANVAS_DURATION 60 * 60 * 24
#define MAX_COORDINATE_X_PLUS_ONE 1000
#define MAX_COORDINATE_Y_PLUS_ONE 1000
#define PIXELS_PER_ROW 50

#define PATRON_BONUS_PERCENTAGE_POINTS 40
#define POT_PERCENTAGE_POINTS 25

// number of pixels sold anything may be withdrawn
#define WITHDRAW_PIXELS_THRESHOLD 150000

#define REFERRER_PERCENTAGE_POINTS 8
// TEAM_PERCENTAGE == 27

struct st_transferContext {
  account_name purchaser;
  account_name referrer;

  // Fund available for purchasing
  uint64_t amountLeft;  // 1 EOS = 10,000

  // Total fees collected (scaled)
  uint128_t totalFeesScaled;

  // How much is paid to the previous owners
  // uint128_t totalPaidToPreviousOwnersScaled;

  // How many pixels are painted
  uint64_t paintedPixelCount;

  uint128_t amountLeftScaled() { return amountLeft * PRECISION_BASE; }

  st_buyPixel_result purchase(const pixel &pixel,
                              const st_pixelOrder &pixelOrder) {
    auto result = st_buyPixel_result();

    auto isBlank = pixel.isBlank();
    result.isFirstBuyer = isBlank;

    if (!isBlank && pixelOrder.color == pixel.color) {
      // Pixel already the same color. Skip.
      result.isSkipped = true;
      return result;
    }

    if (!isBlank && pixelOrder.priceCounter < pixel.nextPriceCounter()) {
      // Payment too low for this pixel, maybe price had increased (somebody
      // else drew first). Skip.
      result.isSkipped = true;
      return result;
    }

    uint64_t nextPrice = pixel.nextPrice();
    eosio_assert(amountLeft >= nextPrice, "insufficient fund to buy pixel");

    if (isBlank) {
      // buying blank. The fee is the entire buy price.
      result.feeScaled = nextPrice * PRECISION_BASE;
    } else {
      // buying from another player. The fee is a percentage of the gain.
      uint64_t currentPrice = pixel.currentPrice();
      uint128_t priceIncreaseScaled =
          (nextPrice - currentPrice) * PRECISION_BASE;

      result.feeScaled = priceIncreaseScaled * FEE_PERCENTAGE / 100;
      result.ownerEarningScaled = nextPrice * PRECISION_BASE - result.feeScaled;
    }

    // bookkeeping for multiple purchases
    amountLeft -= nextPrice;
    paintedPixelCount++;
    totalFeesScaled += result.feeScaled;

    return result;
  }

  uint128_t patronBonusScaled;
  uint128_t potScaled;
  uint128_t teamScaled;
  uint128_t referralEarningScaled;

  bool hasReferrer() { return referrer != 0; }

  void updateFeesDistribution() {
    patronBonusScaled = totalFeesScaled * PATRON_BONUS_PERCENTAGE_POINTS / 100;

    potScaled = totalFeesScaled * POT_PERCENTAGE_POINTS / 100;

    referralEarningScaled = totalFeesScaled * REFERRER_PERCENTAGE_POINTS / 100;
    if (referrer == 0) {
      // if no referrer, pay the pot.
      potScaled += referralEarningScaled;
      referralEarningScaled = 0;
    }

    teamScaled =
        totalFeesScaled - patronBonusScaled - potScaled - referralEarningScaled;
  }

  uint128_t canvasMaskScaled;
  uint128_t bonusPerPixelScaled;

  void updateCanvas(canvas &cv) {
    cv.potScaled += potScaled;
    cv.teamScaled += teamScaled;
    cv.pixelsDrawn += paintedPixelCount;

    bonusPerPixelScaled = patronBonusScaled / cv.pixelsDrawn;
    eosio_assert(bonusPerPixelScaled > 0, "bonus is 0");

    cv.maskScaled += bonusPerPixelScaled;
    eosio_assert(cv.maskScaled >= bonusPerPixelScaled, "canvas mask overflow");

    canvasMaskScaled = cv.maskScaled;
  }

  void updatePurchaserAccount(account &acct) {
    if (amountLeft) {
      acct.balanceScaled += amountLeft * PRECISION_BASE;
    }

    uint128_t patronBonusScaled = bonusPerPixelScaled * paintedPixelCount;

    // FIXME: give the truncated to the team?

    uint128_t maskUpdate =
        canvasMaskScaled * paintedPixelCount - patronBonusScaled;
    acct.maskScaled += maskUpdate;

    eosio_assert(acct.maskScaled >= maskUpdate, "player mask overflow");
    acct.pixelsDrawn += paintedPixelCount;
  }
};